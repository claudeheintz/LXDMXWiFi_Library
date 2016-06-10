/**************************************************************************/
/*!
    @file     ESP-DMX.ino
    @author   Claude Heintz
    @license  BSD (see LXDMXWiFi.h)
    @copyright 2015-2016 by Claude Heintz All Rights Reserved

    Example using LXDMXWiFi_Library for output of Art-Net or E1.31 sACN from
    ESP 8266 WiFi connection to serial DMX.  Or, input from DMX to the network.
    Allows remote configuration of WiFi connection and protocol settings.
    
    Art-Net(TM) Designed by and Copyright Artistic Licence (UK) Ltd
    sACN E 1.31 is a public standard published by the PLASA technical standards program
    
    Note:  For sending packets larger than 512 bytes, ESP8266 WiFi Library v2.1
           is necessary.
           
           This example requires the LXESP8266DMX library for DMX serial output
           https://github.com/claudeheintz/LXESP8266DMX

    @section  HISTORY

    v1.0 - First release
    v2.0 - Added remote configuration with DMXWiFiConfig packets
           Configure by holding pin 16 low on power up to force access point mode.
           Use python script esp_dmx_wifi_config.py to examine & change protocol and connection settings
    v3.0 - Renamed ESP-DMX, Combines output from network and input to network with remote configuration.
           Adds Java version of remote configuration utility.
    v3.1 - Updated for passing pointer to UDP object to LXDMXWiFi_Library functions.
           Moved remote config definitions and functions to separate files
*/
/**************************************************************************/

#include <LXESP8266UARTDMX.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "LXDMXWiFi.h"
#include <LXWiFiArtNet.h>
#include <LXWiFiSACN.h>
#include <EEPROM.h>
#include "LXDMXWiFiConfig.h"

#define STARTUP_MODE_PIN 16      // pin for force default setup when low (use 10k pullup to insure high)
#define DIRECTION_PIN 4          // pin for output direction enable on MAX481 chip

/*         
 *  Edit the LXDMXWiFiConfig.initConfig() function in LXDMXWiFiConfig.cpp to configure the WiFi connection and protocol options
*/

// dmx protocol interface for parsing packets (created in setup)
LXDMXWiFi* interface;

// An EthernetUDP instance to let us send and receive UDP packets
WiFiUDP wUDP;

// used to toggle indicator LED on and off
uint8_t led_state = 0;

// direction output from network/input to network
uint8_t dmx_direction = 0;

// received slots when inputting dmx to network
int got_dmx = 0;


/* 
   utility function to toggle indicator LED on/off
*/
void blinkLED() {
  if ( led_state ) {
    digitalWrite(BUILTIN_LED, HIGH);
    led_state = 0;
  } else {
    digitalWrite(BUILTIN_LED, LOW);
    led_state = 1;
  }
}

/* 
   artAddress callback allows storing of config information
   artAddress may or may not have set this information
   but relevant fields are copied to config struct and stored to EEPROM
*/
void artAddressReceived() {
  DMXWiFiConfig.setArtNetUniverse( ((LXWiFiArtNet*)interface)->universe() );
  DMXWiFiConfig.setNodeName( ((LXWiFiArtNet*)interface)->longName() );
  DMXWiFiConfig.commitToPersistentStore();
}


/*
  DMX input callback function sets number of slots received by ESP8266DMX
*/

void gotDMXCallback(int slots) {
  got_dmx = slots;
}


/************************************************************************

  Setup creates the WiFi connection.
  
  It also creates the network protocol object,
  either an instance of LXWiFiArtNet or LXWiFiSACN.
  
  if OUTPUT_FROM_NETWORK_MODE:
     Starts listening on the appropriate UDP port.
  
     And, it starts the ESP8266DMX sending serial DMX via the UART1 TX pin.
     (see the LXESP8266DMX library documentation for driver details)
     
   if INPUT_TO_NETWORK_MODE:
     Starts ESP8266DMX listening for DMX ( received as serial on UART0 RX pin. )

*************************************************************************/

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(1); //use uart0 for debugging
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(STARTUP_MODE_PIN, INPUT);
  pinMode(DIRECTION_PIN, OUTPUT);
  
  DMXWiFiConfig.begin(digitalRead(STARTUP_MODE_PIN));
  
  dmx_direction = ( DMXWiFiConfig.inputToNetworkMode() );
  
  if ( dmx_direction == OUTPUT_FROM_NETWORK_MODE ) {					      // DMX Driver startup based on direction flag
    Serial.println("starting DMX");
    ESP8266DMX.setDirectionPin(DIRECTION_PIN);
    ESP8266DMX.startOutput();
  } else {
    Serial.println("starting DMX input");
    ESP8266DMX.setDirectionPin(DIRECTION_PIN);
    ESP8266DMX.setDataReceivedCallback(&gotDMXCallback);
    ESP8266DMX.startInput();
  }

  if ( DMXWiFiConfig.APMode() ) {            // WiFi startup
    Serial.print("AP_MODE ");
    Serial.print(DMXWiFiConfig.SSID());
    WiFi.mode(WIFI_AP);
    WiFi.softAP(DMXWiFiConfig.SSID());
    WiFi.softAPConfig(DMXWiFiConfig.apIPAddress(), DMXWiFiConfig.apGateway(), DMXWiFiConfig.apSubnet());
    Serial.print("created access point ");
    Serial.print(DMXWiFiConfig.SSID());
    Serial.print(", ");
  } else {
    Serial.print("wifi connecting... ");
    WiFi.mode(WIFI_STA);
    WiFi.begin(DMXWiFiConfig.SSID(),DMXWiFiConfig.password());

    // static IP otherwise uses DHCP
    if ( DMXWiFiConfig.staticIPAddress() ) {  
      WiFi.config(DMXWiFiConfig.stationIPAddress(), DMXWiFiConfig.stationGateway(), DMXWiFiConfig.stationSubnet());
    }
    
    while (WiFi.status() != WL_CONNECTED) {
      delay(100);
      blinkLED();
    }
  }
  Serial.println("wifi started.");
  
  if ( DMXWiFiConfig.sACNMode() ) {         // Initialize network<->DMX interface
    interface = new LXWiFiSACN();
    interface->setUniverse(DMXWiFiConfig.sACNUniverse());
  } else {
    interface = new LXWiFiArtNet(WiFi.localIP(), WiFi.subnetMask());
    ((LXWiFiArtNet*)interface)->setSubnetUniverse(DMXWiFiConfig.artnetSubnet(), DMXWiFiConfig.artnetUniverse());
    ((LXWiFiArtNet*)interface)->setArtAddressReceivedCallback(&artAddressReceived);
    char* nn = DMXWiFiConfig.nodeName();
    if ( nn[0] != 0 ) {
      strcpy(((LXWiFiArtNet*)interface)->longName(), nn);
    }
  }
  Serial.print("interface created, ");
  
  // if output from network, start wUDP listening for packets
  if ( dmx_direction == OUTPUT_FROM_NETWORK_MODE ) {	
	  if ( DMXWiFiConfig.multicastMode() ) { // Start listening for UDP on port
		 if ( DMXWiFiConfig.APMode() ) {
			wUDP.beginMulticast(WiFi.softAPIP(), DMXWiFiConfig.multicastAddress(), interface->dmxPort());
		 } else {
			wUDP.beginMulticast(WiFi.localIP(), DMXWiFiConfig.multicastAddress(), interface->dmxPort());
		 }
	  } else {
		 wUDP.begin(interface->dmxPort());
	  }
	  Serial.print("udp started,");

	  if ( DMXWiFiConfig.artnetMode() ) { //if needed, announce presence via Art-Net Poll Reply
		  ((LXWiFiArtNet*)interface)->send_art_poll_reply(&wUDP);
	  }
  }

  Serial.println(" setup complete.");
  blinkLED();
} //setup

/************************************************************************

  Main loop
  
  if OUTPUT_FROM_NETWORK_MODE:
    checks for and reads packets from WiFi UDP socket
    connection.  readDMXPacket() returns true when a DMX packet is received.
    In which case, the data is copied to the ESP8266DMX object which is driving
    the UART serial DMX output.
  
    If the packet is an ESP-DMX packet, the config struct is modified and stored in EEPROM
  
  if INPUT_TO_NETWORK_MODE:
    if serial dmx has been received, sends an sACN or Art-Net packet containing the dmx data.
    Note:  does not listen for incoming packets for remote configuration in this mode.

*************************************************************************/

void loop() {
  if ( dmx_direction == OUTPUT_FROM_NETWORK_MODE ) {	
	  uint8_t good_dmx = interface->readDMXPacket(&wUDP);

	  if ( good_dmx ) {
		  for (int i = 1; i <= interface->numberOfSlots(); i++) {
			  ESP8266DMX.setSlot(i , interface->getSlot(i));
		  }
		  blinkLED();
	  } else {
	    if ( strcmp(CONFIG_PACKET_IDENT, (const char *) interface->packetBuffer()) == 0 ) {  //match header to config packet
      	Serial.print("config packet received, ");
		   uint8_t reply = 0;
		   if ( interface->packetBuffer()[8] == '?' ) {  //packet opcode is query
			  reply = 1;
		   } else if (( interface->packetBuffer()[8] == '!' ) && (interface->packetSize() >= DMXWiFiConfigMinSIZE)) { //packet opcode is set
			  DMXWiFiConfig.copyConfig( interface->packetBuffer(), interface->packetSize());
			  DMXWiFiConfig.commitToPersistentStore();
			  reply = 1;
		   } else {
			  Serial.println("packet error.");
		   }
		   if ( reply) {
			  DMXWiFiConfig.hidePassword();                  //don't transmit password!
			  wUDP.beginPacket(wUDP.remoteIP(), interface->dmxPort());
			  wUDP.write((uint8_t*)DMXWiFiConfig.config(), DMXWiFiConfigSIZE);
			  wUDP.endPacket();
			  Serial.println("reply complete.");
			  DMXWiFiConfig.restorePassword();
		   } 
		   interface->packetBuffer()[0] = 0; //insure loop without recv doesn't re-trgger
		   for(int j=0;j<3;j++) {
		   	blinkLED();
		   	delay(100);
		   }
      } // packet has config packet header
	  }   // not good_dmx
	  
	} else {		//direction is input to network
	
	  if ( got_dmx ) {
		 interface->setNumberOfSlots(got_dmx);  //got_dmx
     
		 for(int i=1; i<=got_dmx; i++) {
			interface->setSlot(i, ESP8266DMX.getSlot(i));
		 }
     
		 if ( DMXWiFiConfig.multicastMode() ) {
			 if ( DMXWiFiConfig.APMode() ) {
				 interface->sendDMX(&wUDP,  DMXWiFiConfig.inputAddress(), WiFi.softAPIP());
			 } else {
				 interface->sendDMX(&wUDP,  DMXWiFiConfig.inputAddress(), WiFi.localIP());
			 }
		 } else {
			 interface->sendDMX(&wUDP,  DMXWiFiConfig.inputAddress(), INADDR_NONE);
		 }
		 got_dmx = 0;
		 blinkLED();
	  } // got_dmx
	}   // INPUT_TO_NETWORK_MODE
	
} //loop
