/**************************************************************************/
/*!
    @file     WiFi2DMX.ino
    @author   Claude Heintz
    @license  BSD (see LXDMXWiFi.h)
    @copyright 2015-2016 by Claude Heintz All Rights Reserved

    Example using LXEDMXWiFi_Library for output of Art-Net or E1.31 sACN from
    ESP 8266 WiFi connection to DMX
    
    Art-Net(TM) Designed by and Copyright Artistic Licence (UK) Ltd
    sACN E 1.31 is a public standard published by the PLASA technical standards program
    
    Note:  For sending packets larger than 512 bytes, ESP8266 WiFi Library v2.1
           is necessary.
           
           This example requires the LXESP8266DMX library for DMX serial output
           https://github.com/claudeheintz/LXESP8266DMX

    @section  HISTORY

    v1.00 - First release
    v1.01 - Updated for change to LXESP8266UARTDMX library
*/
/**************************************************************************/

#include <LXESP8266UARTDMX.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <LXDMXWiFi.h>
#include <LXWiFiArtNet.h>
#include <LXWiFiSACN.h>

// *** modify the following for setting up the WiFi connection ***
const char* ssid = "ESP2DMX";
const char* pwd = "***";
uint8_t make_access_point = 1;
uint8_t chan = 2;

// *** modify the following for setting up the network protocol ***
//     Set use_sacn = 0 for Art-Net
uint8_t use_sacn = 1;
uint8_t use_multicast = 1;

// dmx protocol interface for parsing packets (created in setup)
LXDMXWiFi* interface;

// An EthernetUDP instance to let us send and receive UDP packets
WiFiUDP wUDP;

uint8_t led_state = 0;

void blinkLED() {
  if ( led_state ) {
    digitalWrite(BUILTIN_LED, HIGH);
    led_state = 0;
  } else {
    digitalWrite(BUILTIN_LED, LOW);
    led_state = 1;
  }
}

/************************************************************************

  Setup creates the WiFi connection.
  
  It also creates the network protocol object,
  either an instance of LXWiFiArtNet or LXWiFiSACN.
  
  It then starts listening on the appropriate UDP port.
  
  And, it starts the dmx_driver object sending serial DMX via the UART1 TX pin.
  (see the LXESP8266DMX library documentation for driver details)

*************************************************************************/

void setup() {
  Serial.begin(112500);
  Serial.setDebugOutput(1); //use uart0 for debugging
  pinMode(BUILTIN_LED, OUTPUT);
  
  if ( use_sacn == 0 ) {
    use_multicast = 0;    //multicast not used with Art-Net
  }

  if ( make_access_point ) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid);
    WiFi.softAPConfig(IPAddress(10,110,115,10), IPAddress(10,1,1,1), IPAddress(255,0,0,0));
  } else {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid,pwd);

    // static IP for Art-Net  may need to be edited for a particular network
    if ( use_multicast == 0 ) {  
      WiFi.config(IPAddress(10,110,115,15), IPAddress(192,168,1,1), IPAddress(255,0,0,0));
    }
    
    while (WiFi.status() != WL_CONNECTED) {
		delay(100);
		blinkLED();
	 }
  }

  if ( use_sacn ) {                       // Initialize Interface (defaults to first universe)
    interface = new LXWiFiSACN();
    //interface->setUniverse(1);	         // for different universe, change this line and the multicast address below
  } else {
    interface = new LXWiFiArtNet(WiFi.localIP(), WiFi.subnetMask());
    //((LXWiFiArtNet*)interface)->setSubnetUniverse(0, 0);  //for different subnet/universe, change this line
  }

  if ( use_multicast ) {                  // Start listening for UDP on port
    if ( make_access_point ) {
      wUDP.beginMulticast(WiFi.softAPIP(), IPAddress(239,255,0,1), interface->dmxPort());
    } else {
      wUDP.beginMulticast(WiFi.localIP(), IPAddress(239,255,0,1), interface->dmxPort());
    }
  } else {
    wUDP.begin(interface->dmxPort());
  }

  //announce presence via Art-Net Poll Reply
  if ( ! use_sacn ) {
     ((LXWiFiArtNet*)interface)->send_art_poll_reply(wUDP);
  }

  ESP8266DMX.startOutput();

  Serial.println("\nsetup complete");
  blinkLED();
}

/************************************************************************

  The main loop checks for and reads packets from WiFi UDP socket
  connection.  readDMXPacket() returns true when a DMX packet is received.
  In which case, the data is copied to the dmx_driver object which is driving
  the UART serial DMX output.

*************************************************************************/

void loop() {
  uint8_t good_dmx = interface->readDMXPacket(wUDP);

  if ( good_dmx ) {
     for (int i = 1; i <= interface->numberOfSlots(); i++) {
        ESP8266DMX.setSlot(i , interface->getSlot(i));
     }
     blinkLED();
  }
}
