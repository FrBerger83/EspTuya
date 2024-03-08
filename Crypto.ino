/*
  Crypto functions for 55aa (v34) and 6699 (v35) messages
  
    Using the Crypto library by Rhys Weatherley

    Crypting/Decrypting  V3.4 is AES 128 ECB  : Provide Key (16c) + Clear/Crypted Text ( 16b multiple ) return Crypted/Clear Text ( same legth )
      Test 55AA msg       http://aes.online-domain-tools.com/ to test 55AA messages


    Crypting V3.5 is  AES 128 GCM avec IV  : 
        Provide Key (16c) +  Clear Text ( any length ) + Initial vector (12c) + eventually additionnal Data ( 14c prefix ) or nothing
            return : Crypted  Text (same legth) 
                              + Tag (32c)

    DeCrypting V3.5 is  AES 128 GCM avec IV  : 
        Provide Key (16c) +  Crypted Text ( any length ) + Initial vector (12c) 
            return : Clear  Text ( same legth )   

*/
 
//-------------------------------------------------
void decrypt_v34(  )  {         // Parameters None , setup Crypt34 with CryptT = Crypted Text, Key, len => Result in ClearT
    
//creating an object of AES128 class
AES128 aes128;

    byte BlocCR[16] ;
    byte BlocDec[16] ;
  
    DEBUG_PRINTLN("");
    DEBUG_PRINTLN("==== Decrypt v3.4 : AES 128 ECB ");
    DEBUG_PRINT("Crypted    : " ); p5( Cryp34.cryptT, Cryp34.len );
    DEBUG_PRINT("Crypte Key : " ); p5( Cryp34.key, KEY_LENGTH ); 
  
    aes128.setKey( Cryp34.key, KEY_LENGTH );        // Setting Key for AES
    
    int nbb = Cryp34.len / 16 ;
    for ( int i = 0 ; i < nbb ; i++ ){
    
        for (int j = 0 ; j < 16 ; j++ ) { BlocCR[j] = Cryp34.cryptT[(i*16)+j]   ; }
        aes128.decryptBlock( BlocDec, BlocCR  );
        for (int j = 0 ; j < 16 ; j++ ) { Cryp34.clearT[(i*16)+j] = BlocDec[j]  ; }
    } 

    DEBUG_PRINT("DecCrypted : " ); p5( Cryp34.clearT , Cryp34.len ); 
    for (unsigned int i=0; i < Cryp34.len; i++ ) {   
      DEBUG_PRINT( Cryp34.clearT[i] );
    }
    DEBUG_PRINTLN("");
}
 
//-------------------------------------------------
void encrypt_v34( byte* buf, byte *key,  int len )  {  // Parameters None , setup Crypt34 with ClearT  = Clear Text, Key, len => Result in CryptT
    
//creating an object of AES128 class
AES128 aes128;

    byte BlocCR[16] ;
    byte BlocDec[16] ;

    DEBUG_PRINTLN("");
    DEBUG_PRINTLN("==== Encrypt v3.4 : AES 128 ECB ");
    DEBUG_PRINT("deCrypted  : " ); p5( buf, len ); 
    DEBUG_PRINT("Crypte Key : " ); p5( key, KEY_LENGTH ); 
  
    aes128.setKey( key, KEY_LENGTH );// Setting Key for AES
    
    int nbb = len / 16 ;
    for (int i = 0 ; i < nbb ; i++ ){
    
        for (int j = 0 ; j < 16 ; j++ ) { BlocDec[j] = buf[(i*16)+j]   ; }
        aes128.encryptBlock( BlocCR, BlocDec  );
        for (int j = 0 ; j < 16 ; j++ ) { buffer[(i*16)+j] = BlocCR[j]  ; }
    } 

    memcpy( buf, buffer, len );         // on crypte sur place

    DEBUG_PRINT("Crypted    : " ); p5( buf , len ); 
    for (unsigned int i=0; i < len; i++ ) {   
      DEBUG_PRINT(buf[i]);
    }
    DEBUG_PRINTLN("");
}
  
//-------------------------------------------------
void decrypt_v35(  ) {        // Parameters None , setup Cryp35 with CryptT = Crypted Text, IV = Initial vector, Key, len => Result in ClearT
  
    DEBUG_PRINTLN("");
    DEBUG_PRINTLN("==== Decrypt v3.5 : AES 128 GCM avec IV et TAG");
    DEBUG_PRINT("Crypted    : " ); p5( Cryp35.cryptT, Cryp35.len ); 
    DEBUG_PRINT("IV         : " ); p5( Cryp35.IV, 12 ); 
    DEBUG_PRINT("Crypt Key  : " ); p5( Cryp35.key, KEY_LENGTH ); 
  
    GCM<AES128> *gcmaes128 = 0;
    gcmaes128 = new GCM<AES128>();
    gcmaes128->setKey(  Cryp35.key, KEY_LENGTH );
    gcmaes128->setIV(   Cryp35.IV, 12 );
    gcmaes128->decrypt( Cryp35.clearT, Cryp35.cryptT, Cryp35.len );
    delete gcmaes128;
   
    //memcpy( buf, bufdec, len );         // on decrypte sur place

    DEBUG_PRINT("DecCrypted : " ); p5( Cryp35.clearT , Cryp35.len  ); 
    for (unsigned int i=0; i < len; i++ ) {   
      DEBUG_PRINT(Cryp35.clearT[i]);
    }
    DEBUG_PRINTLN("");
}
 
//-------------------------------------------------
void encrypt_v35(byte* buf, byte *key, byte *IV, int len, byte *add, int ladd) {  // Parameters Buf = Msg to crypt, Key, Initial vector, Length, add=additional data
  
    DEBUG_PRINT("");
    DEBUG_PRINTLN("==== Encrypt v3.5 : AES 128 GCM avec IV et TAG");
    DEBUG_PRINT("Clear      : " ); p5( buf, len ); 
    DEBUG_PRINT("IV         : " ); p5( IV, 12 );    // ou memcopy IV, buf, 12   ???
    DEBUG_PRINT("Crypt  Key : " ); p5( key, KEY_LENGTH ); 
  
    GCM<AES128> *gcmaes128 = 0;
    gcmaes128 = new GCM<AES128>(); 
   
    gcmaes128->setKey( key, KEY_LENGTH );
    gcmaes128->setIV( IV, 12);    
    gcmaes128->addAuthData( add, ladd );
    gcmaes128->encrypt( buffer,  buf, len ); 
    gcmaes128->computeTag( wTag, 16);    
    delete gcmaes128;  

    memcpy( buf, buffer, len );         // on decrypte sur place

    DEBUG_PRINT("Crypted    : " ); p5( buf , len );  
    DEBUG_PRINT("Tag        : " ); p5(  wTag, 16 );  
    DEBUG_PRINTLN("");
}
  
//-------------------------------------------------
void genSHA256Key( byte key[], byte fromk[], byte tok[], int len ) {

SHA256 sha256; 

    DEBUG_PRINTLN("");
    sha256.resetHMAC( key, KEY_LENGTH );
    sha256.update( fromk, len );
    sha256.finalizeHMAC( key, KEY_LENGTH, tok, SHA256HMAC_SIZE );

    DEBUG_PRINTLN("------- Crypting CRC Hash SHA256 ---------------");
    DEBUG_PRINT("Key  : " ); p5( key  , KEY_LENGTH );  
    DEBUG_PRINT("From : " ); p5( fromk, len );  
    DEBUG_PRINT("To   : " ); p5( tok  , SHA256HMAC_SIZE );  
    DEBUG_PRINTLN("----------------------");
}

//--------------------------------------------------
void genXORkey() {
  /*       First take local_nonce         30313233343536373839616263646566       
            then self.remote_nonce        39316132646630626235396364623133        
            XOR between 2                 09005301505306555A0C580107065455
            Crypt result with dev Key     485c62...................6f35441  => That is the new key( Xor_Key )
  */
    for (int i=0; i < 16; i++)   { Xor_key[i] = local_nonce[i]  ^ remote_nonce[i] ;   }

    //------------ On crypte  AES  la nouvelle clé reçue
    if ( dev.vers == 4 ){

        encrypt_v34( Xor_key, dev.key, 16 );
    }
    if ( dev.vers == 5 ){

        memcpy( wIV,  local_nonce, 12 ) ;                                 //   IV = local nonce
        encrypt_v35( Xor_key, dev.key, wIV,  16, Suf35, 0 )  ;            //   Suf35 not used?, no additional data  
    }

    DEBUG_PRINTLN("----------------------");
    DEBUG_PRINT("New crypted Key : " ); p5( Xor_key, 16 );  
    DEBUG_PRINTLN("----------------------");
}


