/**************************************************************************/
/*!
    @file     WiFi2Universes.ino
    @author   Claude Heintz
    @license  BSD (see LXDMXWiFi.h)
    @copyright 2015-2016 by Claude Heintz All Rights Reserved

    Example using LXEDMXWiFi_Library for output of Art-Net or E1.31 sACN from
    ESP 8266 WiFi connection to DMX
    
    Art-Net(TM) Designed by and Copyright Artistic Licence Holdings Ltd.
    sACN E 1.31 is a public standard published by the PLASA technical standards program
    
    Note:  For sending packets larger than 512 bytes, ESP8266 WiFi Library v2.1
           is necessary.
           
           This example requires the LXESP8266DMX library for DMX serial output
           https://github.com/claudeheintz/LXESP8266DMX

    @section  HISTORY

    v1.00 - First release
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
//     Set use_sacn = 0 for Art-Net; use_sacn = 1 for E1.31 sACN
uint8_t use_sacn = 0;
uint8_t use_multicast = 1;

// dmx protocol interface for parsing packets (created in setup)
LXDMXWiFi* interface;
LXDMXWiFi* interfaceUniverse2;

// buffer large enough to contain incoming packet
uint8_t packetBuffer[SACN_BUFFER_MAX];

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
  
  And, it starts the ESP8266DMX object sending serial DMX via the UART1 TX pin.
  (see the LXESP8266DMX library documentation for driver details)

*************************************************************************/

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(1); //use uart0 for debugging
  pinMode(BUILTIN_LED, OUTPUT);
  
  if ( use_sacn == 0 ) {
    use_multicast = 0;    //multicast not used with Art-Net
  }

  if ( make_access_point ) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid);
    WiFi.softAPConfig(IPAddress(10,110,115,10), IPAddress(10,110,115,10), IPAddress(255,0,0,0));
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
    interface = new LXWiFiSACN(&packetBuffer[0]);
    //interface->setUniverse(1);	         // for different universe, change this line and the multicast address below
    interfaceUniverse2 = new LXWiFiSACN(&packetBuffer[0]);	//Note:  second universe does not work with multicast
    interfaceUniverse2->setUniverse(2);
  } else {
    interface = new LXWiFiArtNet(WiFi.localIP(), WiFi.subnetMask(), &packetBuffer[0]);
    //((LXWiFiArtNet*)interface)->setSubnetUniverse(0, 0);  //for different subnet/universe, change this line
    
    // this interface will handle poll replies, set reply fields for second port
    uint8_t* pollReply = ((LXWiFiArtNet*)interface)->replyData();
    pollReply[173] = 2;    // number of ports
  	 pollReply[175] = 128;  //  can output from network (port2)
  	 pollReply[183] = 128;  //  good output... change if error  (port2)
    pollReply[191] = 1;    //  universe  (port2)
    
    interfaceUniverse2 = new LXWiFiArtNet(WiFi.localIP(), WiFi.subnetMask(), &packetBuffer[0]);
    ((LXWiFiArtNet*)interfaceUniverse2)->setSubnetUniverse(0, 1);
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

  //announce presence via Art-Net Poll Reply (only advertises one universe)
  if ( ! use_sacn ) {
     ((LXWiFiArtNet*)interface)->send_art_poll_reply(&wUDP);
  }

  ESP8266DMX.startOutput();
  pinMode(14, OUTPUT);
  pinMode(12, OUTPUT);

  Serial.println("\nsetup complete");
  blinkLED();
}

/************************************************************************

  The main loop checks for and reads packets from WiFi UDP socket
  connection.  readDMXPacketContents() returns true when a DMX packet is received.
  If the first universe does not match, try the second

*************************************************************************/

void loop() {
  uint16_t packetSize = wUDP.parsePacket();
  if ( packetSize ) {
  	  packetSize = wUDP.read(packetBuffer, SACN_BUFFER_MAX);
	  uint8_t read_result = interface->readDMXPacketContents(&wUDP, packetSize);
	  uint8_t read_result2 = 0;

	  if ( read_result == RESULT_DMX_RECEIVED ) {
	     analogWrite(12,2*interface->getSlot(1));
	  	  // for bandwidth testing, both universes are sent to output
	     // Note:  ESP8266DMX can only output a single universe
		  for (int i = 1; i <= interface->numberOfSlots(); i++) {
			  ESP8266DMX.setSlot(i , interface->getSlot(i));
		  }
	  } else if ( read_result == RESULT_NONE ) {				// if not good_dmx first universe, try 2nd
	     read_result2 = interfaceUniverse2->readDMXPacketContents(&wUDP, packetSize);
	     if ( read_result2 == RESULT_DMX_RECEIVED ) {
	     		// for bandwidth testing, first universe interface is sent to DMX output again
	         // Note:  ESP8266DMX can only output a single universe
	     		for (int i = 1; i <= interface->numberOfSlots(); i++) {
			  	  ESP8266DMX.setSlot(i , interface->getSlot(i));
		  		}
		  		analogWrite(14,2*interfaceUniverse2->getSlot(512));
	     }
	  }
	  if ( read_result || read_result2 ) {
	  	  blinkLED();
	  }
	}
}
