

//-------- High level commandes
//    Get status
//    Turn device On/Off
//    Turn Lamp On/Off  ( not same DPS as device )

//    ... Add your own depending on DPS needed


//--------------------------------------------------
void  getStatus(  ){
  
    Serial.println();
    Serial.println("--  Send Commande 10 : Request status --");
  
    tryConnect(10);       

    //memcpy( Msg_key, dev.key, KEY_LENGTH ); 
    getKey( ) ;
  
    ret = cmdStatus(); 

    _client.stop();
}

//--------------------------------------------------
int turnOn() {

byte Pload1[] = { 0x22, 0x31, 0x22, 0x3a, 0x74, 0x72, 0x75,  0x65 } ;   // "1":true 

    Serial.println();
    Serial.println("--  Send Command OD  DPS1:true  : Turn ON --");
    Serial.println();

    tryConnect(5);

    getKey( ) ;
   
    ret = sendCommande( Pload1, sizeof(Pload1) );

    _client.stop();

    return ret ;
}

//--------------------------------------------------
int turnOff() {

byte Pload1[] = { 0x22, 0x31, 0x22, 0x3a, 0x66, 0x61, 0x6c, 0x73, 0x65 } ;   // "1":false
 
    Serial.println();
    Serial.println("--  Send Command   OD  DPS1:false : Turn OFF --");
    Serial.println();
 
    tryConnect(5);

    getKey( ) ;
  
    ret = sendCommande( Pload1, sizeof(Pload1) );
    
    _client.stop();

    return ret ;
}

//--------------------------------------------------
int lampOn() {

byte Pload1[] = { 0x22, 0x32, 0x30, 0x22, 0x3a, 0x74, 0x72, 0x75,  0x65 } ;   // "20":true 

    Serial.println();
    Serial.println("--  Send Command  OD  DPS20:true  : Lamp ON --");
    Serial.println();

    tryConnect(5);

    getKey( ) ;
   
    ret = sendCommande( Pload1, sizeof(Pload1) );

    _client.stop();

    return ret ;
}

//--------------------------------------------------
int lampOff() {

byte Pload1[] = { 0x22, 0x32, 0x30, 0x22, 0x3a,0x66, 0x61, 0x6c, 0x73, 0x65 } ;   // "20":false 

    Serial.println();
    Serial.println("--  Send Command   OD  DPS20:false : Lamp OFF --");
    Serial.println();

    tryConnect(5);

    getKey( ) ;
   
    ret = sendCommande( Pload1, sizeof(Pload1) );

    _client.stop();

    return ret ;
}

//--------------------------------------------------
int listen() { // Not checked yet!!!!!!!!!
  
    tryConnect(5);
    getKey( ) ;

    Serial.println();
    Serial.println("--  Send Commande  : Listen --");
    Serial.println();
   
    //----------------------- Reception de la réponse
    memcpy( crypt_key, Xor_key, 16 ) ;           // On copie la clé de decryptage
           
    while (1) {
          
        len = receiveMsg( ) ;

        if ( len ) {
            
            DEBUG_PRINTLN("----------------------");
            DEBUG_PRINT("Received listen : " ); p5( incomingPacket, len );  
            DEBUG_PRINTLN("----------------------");
            
            Lmes.TotL = len ;
            InitLmes( 'M' );                            // Init Message structure ( length)4
          
            unpackMsg();
        } 
    }
    _client.stop();
    return ret ;
}




//--------------------------------------------------
int cmdStatus() {           // Send commande 10 status 
       
    byte Pref34[] = { 0x00, 0x00, 0x55, 0xaa,             0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x34 } ; 
    byte m3a35[]  = {                         0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x1e } ;  

    Tmes.Pload[0] = 0x7B ;
    Tmes.Pload[1] = 0x7D ;
    Lmes.PloadL = 2;
  
    if ( dev.vers == 4 ) {
    
        padPload();

        //------------ Crypt   AES  le pload with  new key
        encrypt_v34( Tmes.Pload, Xor_key, 16 );

        //------------ Format prefix + pload 
        outMsgL = 0 ;
        addTooutMsg( Pref34,  sizeof(Pref34) ) ; 
        addTooutMsg( Tmes.Pload, Lmes.PloadL ) ; 

        //------------ Compute SHA256Key on prefix + pload  with  new key
        genSHA256Key( Xor_key, outMsg, SHA256Key, 32 );

        //------------ Finalize Message
        addTooutMsg( SHA256Key, SHA256HMAC_SIZE ) ; 
        addTooutMsg( Suf34,     sizeof( Suf34 ) ) ; 
    }

    if ( dev.vers == 5 ) {
     
        memcpy( wIV,  local_nonce, 12 ) ;                                              //  Initial vector
                
        encrypt_v35( Tmes.Pload, Xor_key, wIV,  Lmes.PloadL, m3a35, 14 ) ;
  
        //------------ Format  message
        outMsgL = 0 ;
        addTooutMsg( head66, sizeof( head66 ) ) ; 
        addTooutMsg( m3a35,  sizeof( m3a35  ) ) ; 
        addTooutMsg( wIV,    sizeof(   wIV  ) ) ; 
        addTooutMsg( Tmes.Pload,  Lmes.PloadL ) ; 
        addTooutMsg( wTag,   sizeof( wTag   ) ) ; 
        addTooutMsg( Suf35,  sizeof( Suf35  ) ) ; 
    }

    DEBUG_PRINTLN("----------------------");
    DEBUG_PRINT("OutMsg 3 : " ); p5( outMsg, outMsgL );  
    DEBUG_PRINTLN("----------------------");
      
    //----------------------- Send message
    sendMsg( outMsg, outMsgL ) ; 
 
    //----------------------- Receive answer
    len = receiveMsg( ) ;
    
    DEBUG_PRINTLN("----------------------");
    DEBUG_PRINT("Received Status : " ); p5( incomingPacket, len );  
    DEBUG_PRINTLN("----------------------");
     
    Lmes.TotL = len ;
    InitLmes( 'M' );                            // Init Message structure ( length) 
    memcpy( crypt_key, Xor_key, 16 ) ;          // On copie la nouvelle clé de decryptage
      
    unpackMsg();

    return len ;
}


//--------------------------------------------------
int sendCommande( byte dps[],  int dpslen  ) {        // Send commande OD DPS update status 
  
    byte Pref34[]   = { 0x00, 0x00, 0x55, 0xaa,             0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x74 } ; 
    byte m4a35[]    = {                         0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x1e } ;  
   
    byte Pload134[] = { 0x33, 0x2E, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00       };  // b'3.4\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00
    byte Pload135[] = { 0x33, 0x2E, 0x35, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00       };  // b'3.5\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00
   
    byte Pload2p[]  = { 0x7b, 0x22, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x63, 0x6f, 0x6c, 0x22, 0x3a, 0x35, 0x2c, 0x22, 0x74, 0x22, 0x3a };  // "protocol":5,"t":
    byte Pload3d[]  = { 0x2c, 0x22, 0x64, 0x61, 0x74, 0x61, 0x22, 0x3a, 0x7b, 0x22, 0x64, 0x70, 0x73, 0x22, 0x3a, 0x7b             };  // ,"data":{"dps":{
    byte Pload4f[]  = { 0x7d, 0x7d, 0x7d } ;  // }}}

    //------------------  Create Pload
    getTime();
    Lmes.PloadL =  0  ;

    if ( dev.vers == 4 ) {  addToPload( Pload134 , sizeof(Pload134) ) ; }   //   Pload1 
    if ( dev.vers == 5 ) {  addToPload( Pload135 , sizeof(Pload135) ) ; }   //   Pload1 

    addToPload( Pload2p , sizeof(Pload2p) ) ;     //   Pload2
    addToPload( timebuf ,              10 ) ;     //   timestamp
    addToPload( Pload3d , sizeof(Pload3d) ) ;     //   Pload3
    addToPload( dps ,              dpslen ) ;     //   dps 
    addToPload( Pload4f , sizeof(Pload4f) ) ;     //   End

    DEBUG_PRINT("Pload  : " ); p5( Tmes.Pload, Lmes.PloadL );  
      
    if ( dev.vers == 4 ) {  
      
        padPload ( );
        //------------ Crypte   AES   pload with new Key
        encrypt_v34( Tmes.Pload, Xor_key, Lmes.PloadL );

        //------------ Format  prefix + pload 
        outMsgL = 0 ;
        addTooutMsg( Pref34,     sizeof(Pref34) ) ;       // Prefix
        addTooutMsg( Tmes.Pload, Lmes.PloadL    ) ;       // Pload

        //------------ Compute SHA256Key on  prefix + pload  with new Key
        genSHA256Key( Xor_key, outMsg, SHA256Key, Lmes.PloadL +16 );

        //------------ Finalize Message 
        addTooutMsg( SHA256Key, SHA256HMAC_SIZE ) ;       // CRC
        addTooutMsg( Suf34,      sizeof(Suf34)  ) ;       // Suffix
    }   
    
    if ( dev.vers == 5 ) {  
        
        //------------------------- Adjust Datalength
        unsigned long DataLen = 0; 
        DataLen = 12 + Lmes.PloadL  + 16 ;                //  DataLen = IV + Msg + Tag = 	12 + 71 + 16 =  99
        m4a35[13] = DataLen ;                             //  ajust length  ok pour Msgs < 256c
       
        memcpy( wIV,  local_nonce, 12 ) ;                                         // Copie IV
                
        encrypt_v35( Tmes.Pload, Xor_key, wIV,  Lmes.PloadL, m4a35, 14 ) ;
  
        //--------------- Format  message
        outMsgL = 0 ;
        addTooutMsg( head66, sizeof( head66 ) ) ;     // Heading
        addTooutMsg( m4a35,  sizeof(  m4a35 ) ) ;     // Preffix
        addTooutMsg( wIV,    sizeof(    wIV ) ) ;     // IV

        addTooutMsg(Tmes.Pload,  Lmes.PloadL  ) ;     // Pload
        addTooutMsg( wTag,    sizeof(  wTag ) ) ;     // Tag
        addTooutMsg( Suf35,   sizeof( Suf35 ) ) ;     // Suffix
    }

    DEBUG_PRINTLN("----------------------");
    DEBUG_PRINT("OutMsg 4 : " ); p5( outMsg, outMsgL );  
    DEBUG_PRINTLN("----------------------");
      
    //----------------------- Send message
    sendMsg( outMsg, outMsgL ) ; 
 

    //----------------------- Process  responses : We can receive 2 messages : Ignored
    //    1 response to the comman 0D with Pload null commande 0D
    //    1 response Comman 08, Heartbeat ??

    //----------------------- Reception de la réponse 1
    len = receiveMsg( ) ;
    
    DEBUG_PRINTLN("----------------------");
    DEBUG_PRINT("Received 4.1 : " ); p5( incomingPacket, len );  
    DEBUG_PRINTLN("----------------------");
     
    Lmes.TotL = len ;
    InitLmes( 'M' );                            // Init Message structure ( length) 
    memcpy( crypt_key, Xor_key, 16 ) ;          // Copie decrypt key
      
    unpackMsg();

    //----------------------- Reception de la réponse 2
    len = receiveMsg( ) ;
      
    DEBUG_PRINTLN("----------------------");
    DEBUG_PRINT("Received 4.2 : " ); p5( incomingPacket, len );  
    DEBUG_PRINTLN("----------------------");
  
    Lmes.TotL = len ;
 
    unpackMsg();
 
    return len ;
}
    




