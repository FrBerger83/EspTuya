/*    
    ESPTuya Test program to access Tuya Devices directly over Wifi without Cloud access.

      Loaded on a ESP8266 Wemos D1 module
      Access Tuya devices : 
        - Led Lamp bulb * 2           :  using protocol V3.5 (6699)
        - Earu Energy monitor Tuya    :  using protocol V3.4 (55AA)
        - Double Energy Monitor       :  using protocol V3.5 (6699)
        - Smart Plug                  :  using protocol V3.4 (55AA)



*/

const char* thisPgm  = "Tuya/EspTuya"  ;


//#define _DEBUG      1

#ifdef _DEBUG
 #define DEBUG_PRINT(x)       Serial.print (x)
 #define DEBUG_PRINTDEC(x)    Serial.print (x, DEC)
 #define DEBUG_PRINTHEX(x)    Serial.print (x, HEX)
 #define DEBUG_PRINTLN(x)     Serial.println (x)
 #define DEBUG_PRINTJSON(x)   serializeJson(doc, Serial);Serial.println()
#else
 #define DEBUG_PRINT(x)
 #define DEBUG_PRINTDEC(x)
 #define DEBUG_PRINTHEX(x)
 #define DEBUG_PRINTLN(x)
 #define DEBUG_PRINTJSON(x)
#endif


//------------------ Include Crypto Libraries : Crypto by Rhys Weatherley
#include <Hash.h>  
#include <Crypto.h>       
#include <AES.h>
#include <GCM.h>
#include <SHA256.h>


#include <ArduinoJson.h>
JsonDocument doc;

//      #include your WIFI library....  

#include <ESP8266WiFi.h>
#include "time.h"

// Replace with your network credentials
const char* ssid = "REPLACE_WITH_YOUR_SSID";
const char* password = "REPLACE_WITH_YOUR_PASSWORD";
....



WiFiUDP           Udp ;
WiFiClient    _client ;
unsigned int   UDPPort   = 6667 ;
unsigned int   DevPort = 6668 ;

#define KEY_LENGTH 16
#define SHA256HMAC_SIZE 32 
#define TEXT_MAX_SIZE   500 

 

//--------------------- Devices structure  
struct sDev {			
		byte   key[ KEY_LENGTH ] ;	// Device key
	  byte   ip[4]  ;           	// IP adress
	  int    vers   ;           	// Version  : 4 = 3.4, 5 = 3.5
}; 


//--------------------- Devices : Insert here  one line per device to test

sDev disj1 = { { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  } , { 192,168, xxx, xx }, 4 }; 
sDev lamp1 = { { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  } , { 192,168, xxx, xx }, 5 }; 
sDev lamp2 = { { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  } , { 192,168, xxx, xx }, 5 }; 
sDev xxxx...
sDev dev ;  // Device selected             



// ----------------------Tuya Message Structures  
struct sTuyaMsg {			      
		byte   Header[  18 ] ;	// Complete Header 
		byte   Prefix[   4 ] ;	// Prefix 0055AA or 006699
		byte   unknown[  2 ] ;	// Unknown in 006699 Msgs
		int    Segnbr        ;	// Sequence  Nbr
		int    Cmd           ;	// Commande
    int    RC            ;	// Return code
  	byte   IV[ 12 ]      ;	// Initial Vector (only 6699 msgs)  
 		byte   Pload[ TEXT_MAX_SIZE ]  ;	// Payload message 
 		byte   CRC[    32 ]  ;	// Le CRC or Tag(only 6699 msgs) 
		byte   Suffix[  4 ]  ;	// Suffix 00AA55 or 009966
}; 

struct sTuyaMsgLength {			 
		int    TotL       ;	//   Total Msg length
 		int    PrefL      ;	//   Prefix length
 	  int    RCL        ;	//   Return Code length
  	int    PloadL     ;	//   Payload length 
  	int    CRCL       ;	//   CRC length
  	int    SufL       ;	//   Suffix length
}; 

sTuyaMsg Tmes ;
sTuyaMsgLength Lmes ;


// ---------------------- Crypto Message Structures  
struct sCryp34 {			                //  Crypto v3.4 Structure for messages 55AA
		byte   clearT[ TEXT_MAX_SIZE ] ;	// Clear Text
		byte   cryptT[ TEXT_MAX_SIZE ] ;	// Crypted text
	  byte   key   [    KEY_LENGTH ] ;	// crypto ket
    int    len                     ;  // Text length
}; 

struct sCryp35 {			                //  Crypto v3.5 Structure for messages 6699
		byte   clearT[ TEXT_MAX_SIZE ] ;	// Clear Text
		byte   cryptT[ TEXT_MAX_SIZE ] ;	// Crypted text
	  byte   key   [    KEY_LENGTH ] ;	// crypto ket
	  byte   IV    [            12 ] ;	// Initial vector
	  byte   tag   [    KEY_LENGTH ] ;	// Tag
    int    len                     ;  // Text length
}; 

sCryp34 Cryp34 ;
sCryp35 Cryp35 ;

byte   wIV    [         12 ] ;	    // working Initial vector
byte   wTag   [ KEY_LENGTH ] ;	    // working Tag 

byte SHA256Key[SHA256HMAC_SIZE]; 



//---------------------------- Messages Fields and Keys

byte head55 [4]  = {  0x00 , 0x00 , 0x55 , 0xAA  };	    // Header 55 to check Heading 3.4
byte head66 [4]  = {  0x00 , 0x00 , 0x66 , 0x99  };	    // Header 66 to check Heading 3.5

byte Suf34[] = {0x00, 0x00, 0xaa, 0x55, }; 
byte Suf35[] = {0x00, 0x00, 0x99, 0x66, }; 

//------- Key fields to crypt/decrypt messages
  
byte local_nonce[]  = { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66 };   //b'0123456789abcdef', never change
byte remote_nonce[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };   // will be sent by device

byte UDP_key[]      = { 0x6C, 0x1E, 0xC8, 0xE2, 0xBB, 0x9B, 0xB5, 0x9A, 0xB5, 0x0B, 0x0D, 0xAF, 0x64, 0x9B, 0x41, 0x0A };   // for UDP Same for all devices, never change
byte crypt_key[]    = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };   // used by crypt/decrypt functions
byte Msg_key[]      = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };   // Change for each Device
byte Xor_key[]      = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };   // New key to communicate with device
 

byte buffer[TEXT_MAX_SIZE] ;          // Generic buffer
byte outMsg[TEXT_MAX_SIZE] ;          // Out message
byte incomingPacket[TEXT_MAX_SIZE];   // In messages
int packetSize = 0 ;
int outMsgL    = 0 ;


//---------------------------- Timing variables
unsigned long  loctimestamp  ;     // local time stamp ( Recup from php get msg = unix time in seconds since 1/1/1970)
unsigned long  lastupdate    ;     // local time stamp ( Recup from php get msg = unix time in seconds since 1/1/1970)
byte timebuf[10] ;
long int t1 = 0  ;
long int t2 = 0  ;

//---------------------------- Miscellaneous variables
int ret     = 0 ;
int len     = 0 ;
int start   = 0 ;
int fin     = 0 ;

//-------------------------------------------------------------
void setup() {

    Serial.begin(115200);

    Serial.println("********** Debut Program **********");
    Serial.println("thisPgm");
    Serial.println("***********************************");
    
//********* Customize hereafter : your LAN access : wifi.Begin etc..

    //------------------------- Access WIFI and provide Unix Timestamp
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);
        Serial.print("Connecting to WiFi ..");
        while (WiFi.status() != WL_CONNECTED
          .............

              
//********* Customize hereafter : setup loctimestamp with Unix time according to your environment

          loctimestamp  = epochTime ....;  
          lastupdate    = millis() ;        
    //------------------------- 





    //-------------------- Scan UDP for all devices available on Wifi network
    Serial.println  ( "*** End Setup : Scan UDP for Devices ***" ) ; 
    int x = FindDevice() ;

}

//-------------------------------------------------------------
void loop() { 

    Serial.println("*********** Loop *********"); 
    

    Serial.println("*********** Test device 1 *********"); 
    t1 = millis();    
    
    setDevice( disj1 );      
    getStatus( );   // turnOff() ;      delay(1000);    turnOn() ;

    t2 = millis(); Serial.print("Exec Time :"); Serial.print(t2-t1); Serial.println(" ms."); t1 = millis(); 
    

    Serial.println("*********** Test device 2 *********"); 
   
    setDevice( lamp1 );      
    getStatus( );    lampOn() ;   delay(1000);    lampOff() ;

    t2 = millis(); Serial.print("Exec Time :"); Serial.print(t2-t1); Serial.println(" ms."); t1 = millis(); 
    

    Serial.println("*********** Test device 3 *********"); 
   
    setDevice( lamp2 );
      
    getStatus( );    lampOn() ;   delay(1000);    lampOff() ;
    t2 = millis(); Serial.print("Exec Time :"); Serial.print(t2-t1); Serial.println(" ms."); t1 = millis(); 
    

    //listen();   Not working yet


    delay(20000);
} 

//-------------------------------------------------------------
void setDevice( sDev &fromDev) {    

    memcpy( dev.key, fromDev.key, KEY_LENGTH  );         
    memcpy( dev.ip,  fromDev.ip,  4           );        
    dev.vers      =  fromDev.vers ;
}

