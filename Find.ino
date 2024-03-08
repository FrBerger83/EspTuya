
/*--------------------- Find devices functions -----------------------------

    Find device on the LAN :

      - Scanning the UDP port
      - Unpacking the messages ( All devices seem to send an UDP message every 5s )
      - decrypting the Payload
          Using the    UDP_key[]= { 0x6C, 0x1E, 0xC8, 0xE2, 0xBB, 0x9B, 0xB5, 0x9A, 0xB5, 0x0B, 0x0D, 0xAF, 0x64, 0x9B, 0x41, 0x0A };   
                                  Apparently the  Same for all devices, never change
                    
     
*/


int FindDevice() {

    int ret = 0 ;

    //----------------------- Begin UDP listenning
    Serial.println();
    Serial.println("===== FindDevices =====");
    DEBUG_PRINTLN("--  Start UDP --");
    
    DEBUG_PRINTLN("--  Listenning UDP --"); 

    Udp.begin( UDPPort );
       
    while ( ret < 4 )                         // ---- Get 4 messages Only, modify if you want more..
    {
        packetSize = Udp.parsePacket();
        if ( packetSize )   {                 //----------------------- Packet received
      
            Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());
            len = Udp.read( incomingPacket, packetSize );
            if ( len > 0 )
            {
                incomingPacket[len] = '\0';
            }

            DEBUG_PRINTLN("UDP packet :"); 
            for( int i = 0; i < len; i++ ) {  p(incomingPacket[i]); } 
            DEBUG_PRINT("<");  DEBUG_PRINTLN("-"); 
           
            ret = ret + 1;

            Lmes.TotL = packetSize ;
            InitLmes( 'U' );                        // Init Message structure ( length)
            memcpy( crypt_key, UDP_key, 16 ) ;      // On copie la clé de decryptage
            unpackMsg();

        }

        packetSize = 0;
    }

    Udp.endPacket();       // Close UDP  
    
    return ret;
}
 