/*  
  Generic functions to process messages


    - Connect, Send, receive
    - Unpack messages : Parse Header, suffix, get messages
    - Pad/ unpad messages
    - Print, etc...


*/

//--------------------------------------------------
void unpackMsg(){

    DEBUG_PRINTLN("----- Unpack Message ----");
    parse_header(); 
    parse_suffix(); 
    getPayload();       

    //---------------------------------- Remove version from Payload
    byte vers[] = { 0x33, 0x2E  }; 

    if ( memcmp ( Tmes.Pload, vers,  2 ) == 0 ){

        start = 0  ;
        for ( int n = 0 ; n < Lmes.PloadL ; n++ ) {  if ( Tmes.Pload[n] == 0x7b ){  start = n ; break ;}  }
        
        DEBUG_PRINT("Version : " ); p5( Tmes.Pload, start ); 
       
        for ( int n = 0 ; n < Lmes.PloadL-start ; n++ ) {  Tmes.Pload[n] = Tmes.Pload[n+start]   ; }   
     
        Lmes.PloadL = Lmes.PloadL - start ;

        DEBUG_PRINT("Json    : " ); p5( Tmes.Pload , Lmes.PloadL  );   
    }

    //------------------------- Print clear Json message

    Tmes.Pload[Lmes.PloadL ] = '\0' ;
         
    String s = (const char*)Tmes.Pload;
    DEBUG_PRINT("Clean Msg : "); Serial.println(s);
    
    // json expand   https://arduinojson.org/v7/api/
    DeserializationError error = deserializeJson( doc, s );
      
    if(error) {
        Serial.print("deserializeJson() returned ");
        Serial.println(error.c_str());
    } else {
      Serial.print("Json Msg : ");
      serializeJsonPretty( doc, Serial );
      Serial.println();
    }
}

//--------------------------------------------------
void  parse_header(){
    /*
             0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 
            00 00 55 aa 00 00 00 00 00 00 00 23 00 00 00 bc 00 00 00 00
            <-Prefix--> <-Seq nbr-> <---Cmnd--> <-meslen--> <-RetCode->    RetCode not always there

              0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 
             00 00 66 99 00 00 00 00 00 00 00 00 00 13 00 00 00 F0
             <-Prefix--> <ukn> <-Seq nbr-> <---Cmnd--> <-meslen--> <-RetCode->    RetCode not always there
    */ 

    DEBUG_PRINTLN("----- Parse_header ----");
    InitMsg() ;
    Lmes.PrefL = 0 ;
    int  uknwl = 0 ;
    
    //------------- Check Prefix 55AA or 6699 
    for ( int i = 0; i < 4; i++ ) { Tmes.Prefix[i] = incomingPacket [i] ; }

    if ( memcmp ( Tmes.Prefix, head55,  4 ) == 0 ) { Lmes.PrefL = 16 ;  } 
    if ( memcmp ( Tmes.Prefix, head66,  4 ) == 0 ) { Lmes.PrefL = 18 ;  uknwl = 2 ; Lmes.CRCL = 16 ; Lmes.RCL =  0 ;} 

    if ( Lmes.PrefL > 0  ) {

        for ( int i = 0; i < Lmes.PrefL; i++ ) { Tmes.Header[i] = incomingPacket [i] ; }  // Get complete Header
  
        int seqnbr = 0 ;
            seqnbr += (unsigned long)incomingPacket[ 4 + uknwl ] << 24;     // Get Message Sequence Nbr
            seqnbr += (unsigned long)incomingPacket[ 5 + uknwl ] << 16;
            seqnbr += (unsigned long)incomingPacket[ 6 + uknwl ] << 8;
            seqnbr += (unsigned long)incomingPacket[ 7 + uknwl ];
        
        Tmes.Segnbr   = seqnbr ;  

        Tmes.Cmd = incomingPacket [ 11 + uknwl ]  ;                         // Get Command

      unsigned long DataLen = 0;                    
            DataLen += (unsigned long)incomingPacket[ 12 + uknwl ] << 24;   // Get Message length
            DataLen += (unsigned long)incomingPacket[ 13 + uknwl ] << 16;
            DataLen += (unsigned long)incomingPacket[ 14 + uknwl ] << 8;
            DataLen += (unsigned long)incomingPacket[ 15 + uknwl ];
          
      Lmes.PloadL  = Lmes.TotL - Lmes.PrefL - Lmes.RCL - Lmes.CRCL  - Lmes.SufL   ;   // Adlust Payload length
  
    } else {
        Serial.println("Invalid header : NO head55 nor head66");
    }
}

//--------------------------------------------------
void  parse_suffix(){
    /*
       0  1  2  3  4  5  6  7   
      0e 90 ca e9 00 00 aa 55
      <---CRC --> <-Suffix ->
      crc 	  : 0e90cae9      Variable according to messages, generaly 4c  or 32c 
      suffix 	: 0000aa55 or 009966
    */
    DEBUG_PRINTLN("----- Parse Suffix ----");
    
    start = Lmes.TotL - Lmes.CRCL - Lmes.SufL ;
    fin   = Lmes.TotL - Lmes.SufL  ;
    for ( int n = 0 ; n < fin - start ; n++ ) {  Tmes.CRC [n] = incomingPacket [ start + n ]     ;   }

    start = Lmes.TotL - Lmes.SufL ;
    fin   = Lmes.TotL   ;
    for ( int n = 0 ; n < fin - start ; n++ ) {  Tmes.Suffix [n] = incomingPacket [ start + n ]     ;   }     
}

//--------------------------------------------------
void  getPayload(){
    
    DEBUG_PRINTLN("----- Get Payload Data ----"); 
    
    int i = 0 ;
    for ( int n = Lmes.PrefL + Lmes.RCL ; n < Lmes.TotL - Lmes.CRCL  - Lmes.SufL   ; n++ ) {
      
        Tmes.Pload[i] = incomingPacket [n] ;
        i = i + 1 ;
    } 
    Lmes.PloadL = i ; 

    if ( memcmp ( Tmes.Prefix, head66,  4 ) == 0 ) { 
       
        for ( int n = 0 ; n < 12             ; n++ ) { Tmes.IV[n]    = Tmes.Pload[n]    ; }     // Save IV
        for ( int n = 0 ; n < Lmes.PloadL-12 ; n++ ) { Tmes.Pload[n] = Tmes.Pload[n+12] ; }     // Move Payload
     
        Lmes.PloadL = Lmes.PloadL - 12 ;
    }

    DumpMsg() ;
 
    //------------------------------- Decrypt Payload
    
    if ( memcmp ( Tmes.Prefix, head55,  4 ) == 0 ) {  
      
        memcpy( Cryp34.cryptT, Tmes.Pload,  Lmes.PloadL ) ;         
        memcpy( Cryp34.key,    crypt_key,    KEY_LENGTH ) ;          
        Cryp34.len = Lmes.PloadL ;

        decrypt_v34( ) ; 

        memcpy( Tmes.Pload, Cryp34.clearT, Lmes.PloadL  ) ;     // decrypt over crypted text         
    }
  
    if ( memcmp ( Tmes.Prefix, head66,  4 ) == 0 ) {  
      
        memcpy( Cryp35.cryptT, Tmes.Pload,  Lmes.PloadL   );    // Payload to decrypt
        memcpy( Cryp35.key , crypt_key, KEY_LENGTH );           // Decryption key
        memcpy( Cryp35.IV , Tmes.IV, 12      );                 // Initial vector
        Cryp35.len = Lmes.PloadL ;                              // message length
  
        decrypt_v35( ) ; 

        memcpy( Tmes.Pload, Cryp35.clearT, Lmes.PloadL  );      // decrypt over crypted text
    }
          
    unpadPload() ;                                              // Remove unecessary caracters

    DEBUG_PRINT("Decrypted : " ); p5( Tmes.Pload, Lmes.PloadL ); 
}

//--------------------------------------------------
void InitMsg() {                // Initialize Tuya message 

  for ( int i = 0 ; i < 18 ; i++ ) { Tmes.Header[i] = 0 ; }
  for ( int i = 0 ; i <  4 ; i++ ) { Tmes.Prefix[i] = 0 ; }
  
	Tmes.Segnbr    = 0;
	Tmes.Cmd       = 0;
 	Tmes.RC        = 0;
  for ( int i = 0 ; i < TEXT_MAX_SIZE; i++ ) { Tmes.Pload[i]  = 0 ; }   
  for ( int i = 0 ; i <  32; i++ ) { Tmes.CRC[i]    = 0 ; }     
  for ( int i = 0 ; i <  12; i++ ) { Tmes.IV[i]     = 0 ; }    
  for ( int i = 0 ; i <   4; i++ ) { Tmes.Suffix[i] = 0 ; }    //memset(Tmes.Suffix,0,sizeof(Tmes.Suffix)); ?
}

//--------------------------------------------------
void InitLmes(char x ) {      // Initialize Tuya message lengths

Lmes.PrefL = 16 ;
Lmes.SufL  =  4 ;

    if ( x ==  'U' ) {        // Message UDP
        Lmes.RCL    =   4 ;
        Lmes.CRCL   =   4 ;
    }
    
     if ( x ==  'M' ) {        // Devices Messages  
        Lmes.RCL    =   4 ;    // ????
        Lmes.CRCL   =  32 ;
    }  
}
 
     
//--------------------------------------------------
void padPload(   ) {        // Vers 3.4 payload have to be multiple of 16

    int x =  Lmes.PloadL % 16 ;                        
    for ( int i = 0 ; i <  16-x; i++ ) { Tmes.Pload[Lmes.PloadL+i] = byte(16-x) ;  }        //  Padding
    Lmes.PloadL = Lmes.PloadL + 16-x;
}
     
//--------------------------------------------------
void unpadPload() {

    //----------------------------------------- Suppress trailing byte padding Version 3.4 only
    if ( Tmes.Pload[Lmes.PloadL-1] <= 16 ) {
        DEBUG_PRINT("Supress trail ( "  ); DEBUG_PRINTHEX( Tmes.Pload[Lmes.PloadL-1] );  DEBUG_PRINT(" ) : "  ); DEBUG_PRINT(Lmes.PloadL); 
        Lmes.PloadL = Lmes.PloadL - Tmes.Pload[Lmes.PloadL-1] ;
        DEBUG_PRINT(" ==> "  ); DEBUG_PRINTLN(Lmes.PloadL); 
    }

    //----------------------------------------- Suppress leading 0s  , Version 3.5 only
    int i = 0; 
    
    while ( Tmes.Pload[i] < 16 ) {  i = i + 1 ;  }        // Count leading 0
    if ( i > 0 ) {
        for ( int n = i ; n < Lmes.PloadL  ; n++ ) {  Tmes.Pload[n-i] = Tmes.Pload[n]   ; }    // Move Pload
       
        DEBUG_PRINT("Supress "  ); DEBUG_PRINT( i );  DEBUG_PRINT(" Leading 0s : "  ); DEBUG_PRINT(Lmes.PloadL); 
        Lmes.PloadL = Lmes.PloadL - i ;
        DEBUG_PRINT(" ==> "  ); DEBUG_PRINTLN(Lmes.PloadL); 
    }
} 
//-------------------------------------------------- Client Device connection
void tryConnect(int nbr) {

char bufip[20];
sprintf(bufip, "IP:%d.%d.%d.%d", dev.ip[0], dev.ip[1], dev.ip[2], dev.ip[3] );

    if ( _client.connected() ) {
        Serial.print( "Already connected port : " ) ;  Serial.println( DevPort ) ;  
    } else {
        for ( int i = 1 ; i <= nbr ; i++ ) { 
            if ( _client.connect( dev.ip, DevPort ) ) {
                Serial.print( "Connect OK  port : " ) ;  Serial.print( DevPort ) ; Serial.print( ", try : ") ; Serial.print(i);
                Serial.print( " IP : ") ; Serial.println( bufip );
                break;
            } else {
                Serial.print("Connect Error port : " ) ;  Serial.print( DevPort ) ; Serial.print( ", try : ") ; Serial.print(i);
                delay(50);
            }
        }
    } 
} 
 

//-------------------------------------------------- Send message to Device
void sendMsg( byte X[], int len) {
  
    // Wait for client to be ready for write

    while ( _client.connected() && _client.availableForWrite() < len ) delay(10);

    DEBUG_PRINT("Ready to write ... : ") ;
    // Write request to device
    int x = _client.write( X, len );
    DEBUG_PRINT(x) ;  DEBUG_PRINTLN(" car writen. ") ;
}
 
//-------------------------------------------------- Receive raw message from Device
int receiveMsg( ) {

    DEBUG_PRINT(" Wait to receive Message. Connected : ") ; DEBUG_PRINTLN( _client.connected()  ) ; 

    // Wait for socket to be ready for read  
    while (_client.connected() && _client.available() < 16 ) delay(10);     // && = AND
 
    len = 0 ;
    while( _client.connected() )                  // loop while the client's connected
    {      
      while( _client.available() )                // if there's bytes to read from the client,
      {
          char ch = static_cast<char>( _client.read() );
          p( ch ) ; 
          incomingPacket[len] = ch;
          len = len + 1;
      }
      if ( len > 10 ) {   // caracters will show after a little while
          DEBUG_PRINTLN("<<") ; DEBUG_PRINT(len) ;  DEBUG_PRINTLN(" car received.") ;
          break;
      }
    }
    return len;
}
 
//--------------------------------------------------
void DumpMsgL() {

    Serial.println("---- Dump Tuya Message Length ----");

    Serial.print(" Long Totale  ") ;  Serial.print( Lmes.TotL  , HEX    );  Serial.print( ", " ); Serial.println( Lmes.TotL    );
    Serial.print(" Long PrefL   ") ;  Serial.print( Lmes.PrefL , HEX    );  Serial.print( ", " ); Serial.println( Lmes.PrefL   );
    Serial.print(" Long RCL     ") ;  Serial.print( Lmes.RCL   , HEX    );  Serial.print( ", " ); Serial.println( Lmes.RCL     );
    Serial.print(" Long PloadL  ") ;  Serial.print( Lmes.PloadL, HEX    );  Serial.print( ", " ); Serial.println( Lmes.PloadL  );
    Serial.print(" Long CRCL    ") ;  Serial.print( Lmes.CRCL  , HEX    );  Serial.print( ", " ); Serial.println( Lmes.CRCL    );
    Serial.print(" Long SufL    ") ;  Serial.print( Lmes.SufL  , HEX    );  Serial.print( ", " ); Serial.println( Lmes.SufL    );
    Serial.println("---- Dump Tuya Message Length Fin ----");   
}
 
//--------------------------------------------------
void DumpMsg() {

    DEBUG_PRINTLN("---- Dump Tuya Message ----");
  
    DEBUG_PRINTLN("------------ ") ; 
    DEBUG_PRINT(" Header "  ) ;  p5( Tmes.Header, Lmes.PrefL);
    DEBUG_PRINT(" Head    " ) ;  p5( Tmes.Prefix,  4 )  ;  
    DEBUG_PRINT(" Segnbr   ") ;	 DEBUG_PRINTHEX( Tmes.Segnbr  );  DEBUG_PRINT( ", " );  DEBUG_PRINTLN( Tmes.Segnbr );   
    DEBUG_PRINT(" Cmd     " ) ;  DEBUG_PRINTHEX( Tmes.Cmd     );  DEBUG_PRINT( ", " );  DEBUG_PRINTLN( Tmes.Cmd   );
    DEBUG_PRINT(" RC       ") ;	 DEBUG_PRINTHEX( Tmes.RC      );  DEBUG_PRINT( ", " );  DEBUG_PRINTLN( Tmes.RC    );   
    DEBUG_PRINT(" Pload "   ) ;  p5( Tmes.Pload , Lmes.PloadL );  
    DEBUG_PRINT(" IV      " ) ;  p5( Tmes.IV, 12);  
    DEBUG_PRINT(" CRC     " ) ;  p5( Tmes.CRC   , Lmes.CRCL   );  
    DEBUG_PRINT(" Suffix  " ) ;  p5( Tmes.Suffix, Lmes.SufL   ); 

    DEBUG_PRINTLN("---- Fin Dump Message ----");
}

//-------------------------------------------------- Update local timestamp
void getTime() {
  
char locbuf[10] ;

    loctimestamp  = loctimestamp  + ( ( millis() - lastupdate ) / 1000 ) ;  // Timestamp adjust (from connection)
    lastupdate    = millis() ; 
    ltoa( loctimestamp, locbuf, 10 ); 
    
    DEBUG_PRINT("---- loctimestamp : "); DEBUG_PRINTLN(loctimestamp);
    DEBUG_PRINT("---- timebuf      : ");  
    for ( int i=0; i < 10   ; i++)
    {
        timebuf[i] = locbuf[i];
        if ( timebuf[i] < 16) {DEBUG_PRINT("0");}
        DEBUG_PRINTHEX( timebuf[i] ) ;
    }
    DEBUG_PRINTLN();   
}
 

//-------------------------------------------------- Add field to Payload
void addToPload(byte X[], int len) {
 
  for (int i=0 ; i < len ; i++   ) { Tmes.Pload[Lmes.PloadL+i] = X[i] ;}   
  Lmes.PloadL =  Lmes.PloadL + len;
}

//-------------------------------------------------- Add field to Output message
void addTooutMsg(byte X[], int len) {
 
  for (int i=0 ; i < len ; i++   ) { outMsg[outMsgL+i] = X[i] ;}   
  outMsgL =  outMsgL + len;
}   

//-------------------------------------------------- Print a byte
void p(byte X) {

   if (X < 16) { DEBUG_PRINT("0"); }
   DEBUG_PRINTHEX(X);
}
   
//-------------------------------------------------- Print a byte field
void p5( byte X[], int s ) {
     
    DEBUG_PRINT(" "); DEBUG_PRINT( s  );  DEBUG_PRINT(" : "); 
    for ( int i=0; i < s   ; i++)
    {
        if ( X[i] < 16) { DEBUG_PRINT("0");}
        DEBUG_PRINTHEX( X[i] ) ;
    }
    DEBUG_PRINTLN();   
}
