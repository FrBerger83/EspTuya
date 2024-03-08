# EspTuya
Arduino Code /ESP32/ESP8266 test program to interface with Tuya WiFi smart devices directly without Cloud access.
 

## Description
This program written with Arduino Code running on ESP8266 (Wemos D1) is just a test to access,  read status and control Tuya compatible WiFi Smart Devices (Plugs, Switches, Lights, Window Covers, etc.) using the local area network (LAN) WITHOUT  accessing the cloud (TuyaCloud API).


I discovered the excellent [Tinytuya](https://github.com/jasonacox/tinytuya)  library on github, however I am not an expert in Python, and wanted to port some of the functionalities to Arduino Code. I was not able to find my way by reading the tons of Tuya documentation, I therefore used TinyTuya to reverse engineer the messages structures and protocol in a way I could understand and port some of it to Arduino code.

It is working with 3.4 Protocol ( 55AA Messages ) and 3.5 Protocol ( 6699 Messages ).


## The Program
The program is made of several .ino files
-	EspTuya.ino :  	The main code
-	Commandes.ino :  	The high level command functions ( Getstatus, On, Off…)
-	Functions.ino :  	All the basic commands ( Unpack messages, send, receive…)
-	Crypto.ino :  	All the crypto function for v3.4 (55AA) and v3.5 (6699) packet formats
-	Find.ino :  	UDP Functions to listen and discover devices
-	GetKey	.ino :  	The protocol negotiation functions
  
I apology to the expert Arduino code programmers, I am not one! and there are certainly more efficient ways to accomplish some of the functions. I wrote it in a way that could be understandable to anyone and avoided on purpose any more efficient shortcuts. My purpose was to demonstrate that it is feasible, and it works for me. 

In order to use it, you need :
-	In the Setup()  function : Customize your WIFI.Begin…  access according to your LAN settings and provide a Unix timestamp to the ‘loctimestamp’ variable.  Here is an example : https://randomnerdtutorials.com/epoch-unix-time-esp32-arduino/

-	Get the Device Key of the device you need to control. You can find detailed instructions on https://github.com/jasonacox/tinytuya  see the chapter : Setup Wizard - Getting Local Keys
([PDF Instructions](https://github.com/jasonacox/tinytuya/files/12836816/Tuya.IoT.API.Setup.v2.pdf))
-	Copy your device info : Key, Ip address and version in a specific structure like that at the beginning of the program :

        sDev disj1 = { { 0x51, 0x74 … 0x67, 0x2b  } , { 192,168, xx, x }, 4 }; 
        sDev lamp1 = { { 0x3c,..x4b, 0x54, 0x73  } , { 192,168, xx, xx }, 5 }; 
        SDev ....
-	Be sure to include the right Crypto library :  The one that worked really well for me :
	        [Crypto Libraries](https://www.arduino.cc/reference/en/libraries/crypto/ )   by Rhys Weatherley
-	In the loop part update the commands you want to perform, Compile and test…

## My environnement
-	A Wemos D1R1 
-	Devices:
-       	2 * Lamp LED Smart Connect (V3.5 Protocol)
        	1 * Earu Energy monitor (V3.4 Protocol)
        	1 * Smart Plug (V3.4 Protocol)
        	1 * Double Energy monitor (V3.5 Protocol)
     

## The main functions used
-	FindDevice() : Scan for Devices on your Lan

-	Related to a specific device

-	    setDevice( disj1 );  Exemple for an energy monitor
 	    getStatus( );    Return all DPS values for the device
 	    turnOff() ;      
 	    turnOn() ;
	
-	    setDevice( lamp1 );  Exemple for a LED Lamp
        getStatus( );    Return all DPS values for the device
        turnOff() ;      (The lamp doesn’t use the standard DPS 1 but 20)
        turnOn() ;


## To be done
Plenty: make the code more efficient, check more thoroughly return codes and unexpected behaviors, make it easier to integrate new devices, improve _Client TCP Communications, new DPS functions… and so on…but I don’t have time on that. I just wanted it to work for me and share it… the basic functions are here, feel free to use and expend.

## Credits

* https://github.com/jasonacox/tinytuya - The Python Tinytuya by Jason Cox 
* https://www.arduino.cc/reference/en/libraries/crypto/ - The Crypto Library by Rhys Weatherley
 