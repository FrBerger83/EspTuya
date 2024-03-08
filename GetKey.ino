
// Negotiation key functions
/*
    In order to command a device, we need to negotiate to obtain a key

    =>  Send msg 1 with     local nonce       0123456789abcdef    crypted using device key   Cmd = 03 Negotiate session key
    <=  Receive msg whith   remote nonce      xxxxxxxxxxxxxxxx    crypted using device key

    =>  Send msg 2 with     remote nonce      xxxxxxxxxxxxxxxx    crypted using device key : Cmd = 05 Finalize session key negotiation (no response expected)
    <=  Receive No msg  

    = Compute new key for future messasges
        XOR between local nonce and remote nonce 
        Crypt result   using device key           
                        =>      New  Xor-key => Once you get it, you can send commands to the devices (status, turn On/Off...)
*/

//--------------------------------------------------
void  getKey(  ){

    ret = 0;
    //----------------------- Begin Get Key
    Serial.println();
    Serial.println("===== Get Key =====");
    Serial.println();  t1 = millis();   
    DEBUG_PRINTLN("--  Send Command 1 : Request remote nonce --");

    ret = reqRemNonce( ) ;      

    DEBUG_PRINT("Remote_nonce : " ); p5( remote_nonce, 16 );  
  
    if (  ret  ) {

      DEBUG_PRINTLN();
      DEBUG_PRINTLN("--  Send Command 2 : Finalize/acknowledge  --");

      ret = finalizeRemNonce();
      
      // Generate XOR Key  (Xor fonction byte par byte between local and remote nonce + crypt )
      genXORkey();
    }  
    DEBUG_PRINTLN();
    DEBUG_PRINTLN("===== Fin Get Key ====="); 
}

//--------------------------------------------------
int  reqRemNonce( ) {      //--  Send Command 1 : Request remote nonce --
                        // Command 03 : Negotiate session key

    byte m1p34[] = { 0x00, 0x00, 0x55, 0xaa, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x44             } ;
    byte m1a35[] = {                         0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x2c } ;
     
    memcpy ( Tmes.Pload, local_nonce, KEY_LENGTH );       // Payload = local_nonce 012...
    Lmes.PloadL = KEY_LENGTH ;
    
    if ( dev.vers == 4 ) {
        
        padPload( ) ;
        encrypt_v34( Tmes.Pload, dev.key, Lmes.PloadL) ;
  
        //------------ Formating output message
        outMsgL = 0 ;
        addTooutMsg( m1p34,     sizeof( m1p34 ) ) ;       // Prefix
        addTooutMsg( Tmes.Pload, Lmes.PloadL    ) ;       // Pload
 
        //------------ Compute SHA256Key  on Prefix + pload  
        genSHA256Key( dev.key, outMsg, SHA256Key, 48 );
    
        //------------ Finalize Message 
        addTooutMsg( SHA256Key, SHA256HMAC_SIZE ) ;    // add CRC
        addTooutMsg( Suf34,      sizeof(Suf34)  ) ;    // Add Suffix
    }

    if ( dev.vers == 5 ) { 
      
        memcpy( wIV,  Tmes.Pload, 12 ) ;                                       // IV = first 12c (local_nonce)
                
        encrypt_v35( Tmes.Pload, dev.key, wIV,  Lmes.PloadL, m1a35, 14 )  ;
  
        //--------------- Format  message
        outMsgL = 0 ;
        addTooutMsg( head66, sizeof( head66 ) ) ;       // Heading
        addTooutMsg( m1a35,  sizeof(  m1a35 ) ) ;       // Prefix
        addTooutMsg( wIV,    sizeof(    wIV ) ) ;       // Initial Vector
        addTooutMsg(Tmes.Pload,   Lmes.PloadL ) ;       // Pload
        addTooutMsg( wTag,    sizeof(  wTag ) ) ;       // Tag
        addTooutMsg( Suf35,   sizeof( Suf35 ) ) ;       // Suffix
    }

    DEBUG_PRINTLN("Message 1 : " ); p5(  outMsg, outMsgL );  
 
    //----------------------- Send message
    sendMsg( outMsg, outMsgL ) ; 
 
    //----------------------- Receive response
    int ret = receiveMsg( ) ;

    Lmes.TotL = ret ;
    InitLmes( 'M' );                         // Init Message structure ( length) 
    memcpy( crypt_key, dev.key, 16 ) ;       // We use Device decrypt Key
            
    parse_header();
    parse_suffix();
    getPayload();  
    
    DumpMsg() ;

    memcpy( remote_nonce, Tmes.Pload, 16) ;    // Get remote_nonce
        
    return 16 ;
}

//--------------------------------------------------
int  finalizeRemNonce()  {    //--  Send Command 2 : Request remote nonce --
                              // Command 05 Finalize session key negotiation

    byte Pref34[] = { 0x00, 0x00, 0x55, 0xaa,             0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x54  } ; 
    byte m2a35[]  = {                         0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x3c  } ; 

    // Compute SHA256Key on  remote_nonce with device key  
    genSHA256Key( dev.key, remote_nonce, SHA256Key, KEY_LENGTH );
 
    //------------ Create Pload  
    memcpy( Tmes.Pload, SHA256Key, SHA256HMAC_SIZE ) ;                     
    Lmes.PloadL = SHA256HMAC_SIZE ;

    if ( dev.vers == 4 ) {
      //  m2l = 100;
        for ( int i = 0 ; i < 16; i++ ) { Tmes.Pload[Lmes.PloadL] = 0x10 ; Lmes.PloadL = Lmes.PloadL + 1; }     // Extend to from 32 to 48c
 
        //------------ Crypt  pload   with Device key
        encrypt_v34( Tmes.Pload, dev.key, 48 );

        //------------ Formating output message
        outMsgL = 0 ;
        addTooutMsg( Pref34,     sizeof( Pref34 ) ) ;       // Prefix
        addTooutMsg( Tmes.Pload, Lmes.PloadL    ) ;         // add Pload
 
        //------------ Compute SHA256Key  on Prefix + pload  
        genSHA256Key( dev.key, outMsg, SHA256Key, 64 );
    
        //------------ Finalize Message 
        addTooutMsg( SHA256Key, SHA256HMAC_SIZE ) ;    // add CRC
        addTooutMsg( Suf34,      sizeof(Suf34)  ) ;    // add Suffix
    }

    if ( dev.vers == 5 ) {
 
        memcpy( wIV,  local_nonce, 12 ) ;                                       // IV = first 12c (local_nonce)
                
        encrypt_v35( Tmes.Pload, dev.key, wIV,  Lmes.PloadL, m2a35, 14 ) ,
  
        //--------------- Format  message
        outMsgL = 0 ;
        addTooutMsg( head66, sizeof( head66 ) ) ;       // Heading
        addTooutMsg( m2a35,  sizeof(  m2a35 ) ) ;       // Prefix
        addTooutMsg( wIV,    sizeof(    wIV ) ) ;       // IV

        addTooutMsg(Tmes.Pload, Lmes.PloadL ) ;         // add Pload
        addTooutMsg( wTag,  sizeof(  wTag ) ) ;         // add Tag
        addTooutMsg( Suf35, sizeof( Suf35 ) ) ;         // add Suffix
    }

    DEBUG_PRINTLN("----------------------");
    DEBUG_PRINT("outMsg 2 : " ); p5( outMsg, outMsgL );  
    DEBUG_PRINTLN("----------------------");
      
    //----------------------- Send messagea acknowledge  
    sendMsg( outMsg, outMsgL  ); 

    // Don't expect any response

    return outMsgL ;
}

