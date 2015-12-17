# LXDMXWiFi_library
Library for ESP8266 implements Art-Net and sACN with example sketches showing both DMX input to and DMX output from network.

LXDMXWiFi encapsulates functionality for sending and receiving DMX over an ESP8266 WiFi connection
   It is a virtual class with concrete subclasses LXWiFiArtNet and LXWiFiSACN which specifically
   implement the Artistic Licence Art-Net and PLASA sACN 1.31 protocols.
   
          
Included examples of the library's use:

         DMX output from network using UART and MAX485 driver chip
         DMX input to network using UART and MAX485 driver chip
