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
*/
/**************************************************************************/

#include <LXESP8266UARTDMX.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "LXDMXWiFi.h"
#include <LXWiFiArtNet.h>
#include <LXWiFiSACN.h>
#include <EEPROM.h>

#define STARTUP_MODE_PIN 16      // pin for force default setup when low (use 10k pullup to insure high)
#define DIRECTION_PIN 4          // pin for output direction enable on MAX481 chip

/*
  config struct with WiFi and Protocol settings, see LXDMXWiFi.h
  Can be modified by remote packet when listening to network
*/
struct DMXWiFiConfig* esp_config;

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
   initConfig initializes the DMXWiFiConfig structure with default settings
   The default is to receive Art-Net with the WiFi configured as an access point.
   (Modify the initConfig to change default settings.  But, highly recommend leaving AP_MODE for default startup.)
*/
void initConfig(DMXWiFiConfig* cfptr) {
  //zero the complete config struct
  uint8_t* p = (uint8_t*) esp_config;
  uint8_t k;
  for(k=0; k<DMXWiFiConfigSIZE; k++) {
    p[k] = 0;
  }
  
  strncpy((char*)cfptr, ESPDMX_IDENT, 8); //add ident
  strncpy(cfptr->ssid, "ESP-DMX-WiFi", 63);
  strncpy(cfptr->pwd, "********", 63);
  cfptr->wifi_mode = AP_MODE;                       // AP_MODE or STATION_MODE
  cfptr->protocol_mode = ARTNET_MODE;     // ARTNET_MODE or SACN_MODE
                                                         // optional | STATIC_MODE or | MULTICAST_MODE or | INPUT_TO_NETWORK_MODE
  cfptr->ap_chan = 2;
  cfptr->ap_address    = IPAddress(10,110,115,10);       // ip address of access point
  cfptr->ap_gateway    = IPAddress(10,110,115,1);
  cfptr->ap_subnet     = IPAddress(255,255,255,0);       // match what is passed to dchp connection from computer
  cfptr->sta_address   = IPAddress(10,110,115,15);       // station's static address for STATIC_MODE
  cfptr->sta_gateway   = IPAddress(192,168,1,1);
  cfptr->sta_subnet    = IPAddress(255,0,0,0);
  cfptr->multi_address = IPAddress(239,255,0,1);         // sACN multicast address should match universe
  cfptr->sacn_universe   = 1;
  cfptr->artnet_universe = 0;
  cfptr->artnet_subnet   = 0;
  strcpy((char*)cfptr->node_name, "com.claudeheintzdesign.esp-dmx");
  cfptr->input_address = IPAddress(10,255,255,255);
}

/* 
   WiFi station password can be set but is never returned by query
*/
void erasePassword(DMXWiFiConfig* cfptr) {
  strncpy(cfptr->pwd, "********", 63);
}

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
  int u = ((LXWiFiArtNet*)interface)->universe();
  esp_config->artnet_universe = u & 0x0F;
  esp_config->artnet_subnet = (u>>4) & 0x0F;
  strncpy((char*)esp_config->node_name, ((LXWiFiArtNet*)interface)->longName(), 31);
  esp_config->node_name[32] = 0;
  esp_config->opcode = 0;
  EEPROM.write(8,0);
  EEPROM.commit();
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
  Serial.begin(112500);
  Serial.setDebugOutput(1); //use uart0 for debugging
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(STARTUP_MODE_PIN, INPUT);
  pinMode(DIRECTION_PIN, OUTPUT);
  
  ESP8266DMX.startOutput();                            // DMX Driver startup
  digitalWrite(DIRECTION_PIN, HIGH);
  
  EEPROM.begin(DMXWiFiConfigSIZE);                      						// Config initialization
  esp_config = (DMXWiFiConfig*)EEPROM.getDataPtr();
  if ( digitalRead(STARTUP_MODE_PIN) == 0 ) {			  						// if startup pin is low, initialize config struct
    initConfig(esp_config);
    Serial.println("\ndefault startup");
  } else if ( strcmp(ESPDMX_IDENT, (const char *) esp_config) != 0 ) {	// if structure not previously stored
    initConfig(esp_config);															// initialize and store in EEPROM
    EEPROM.write(8,0);  //zero term. for ident sets dirty flag 
    EEPROM.commit();
    Serial.println("\nInitialized EEPROM");
  } else {																					// otherwise use the config struct read from EEPROM
    Serial.println("\nEEPROM Read OK");
  }
  
  dmx_direction = ( esp_config->protocol_mode & INPUT_TO_NETWORK_MODE );
  
  if ( dmx_direction == OUTPUT_FROM_NETWORK_MODE ) {					      // DMX Driver startup based on direction flag
    Serial.println("starting DMX");
    ESP8266DMX.startOutput();
    digitalWrite(DIRECTION_PIN, HIGH);
  } else {
    Serial.println("starting DMX input");
    digitalWrite(DIRECTION_PIN, LOW);
    ESP8266DMX.setDataReceivedCallback(&gotDMXCallback);
    ESP8266DMX.startInput();
  }

  if ( esp_config->wifi_mode == AP_MODE ) {            // WiFi startup
    Serial.print("AP_MODE ");
    Serial.print(esp_config->ssid);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(esp_config->ssid);
    WiFi.softAPConfig(esp_config->ap_address, esp_config->ap_gateway, esp_config->ap_subnet);
    Serial.print("created access point ");
    Serial.print(esp_config->ssid);
    Serial.print(", ");
  } else {
    Serial.print("wifi connecting... ");
    WiFi.mode(WIFI_STA);
    WiFi.begin(esp_config->ssid,esp_config->pwd);

    // static IP otherwise uses DHCP
    if ( esp_config->protocol_mode & STATIC_MODE ) {  
      WiFi.config(esp_config->sta_address, esp_config->sta_gateway, esp_config->sta_subnet);
    }
    
    while (WiFi.status() != WL_CONNECTED) {
      delay(100);
      blinkLED();
    }
  }
  Serial.println("wifi started.");
  
  if ( esp_config->protocol_mode & SACN_MODE ) {         // Initialize network<->DMX interface
    interface = new LXWiFiSACN();
    interface->setUniverse(esp_config->sacn_universe);
  } else {
    interface = new LXWiFiArtNet(WiFi.localIP(), WiFi.subnetMask());
    ((LXWiFiArtNet*)interface)->setSubnetUniverse(esp_config->artnet_subnet, esp_config->artnet_universe);
    ((LXWiFiArtNet*)interface)->setArtAddressReceivedCallback(&artAddressReceived);
    if ( esp_config->node_name[0] != 0 ) {
      esp_config->node_name[32] = 0;      //insure zero termination of string
      strcpy(((LXWiFiArtNet*)interface)->longName(), (char*)esp_config->node_name);
    }
  }
  Serial.print("interface created, ");
  
  // if output from network, start wUDP listening for packets
  if ( dmx_direction == OUTPUT_FROM_NETWORK_MODE ) {	
	  if ( ( esp_config->protocol_mode & MULTICAST_MODE ) ) { // Start listening for UDP on port
		 if ( esp_config->wifi_mode == AP_MODE ) {
			wUDP.beginMulticast(WiFi.softAPIP(), esp_config->multi_address, interface->dmxPort());
		 } else {
			wUDP.beginMulticast(WiFi.localIP(), esp_config->multi_address, interface->dmxPort());
		 }
	  } else {
		 wUDP.begin(interface->dmxPort());
	  }
	  Serial.print("udp started,");

	  if ( ( esp_config->protocol_mode & SACN_MODE ) == 0 ) { //if needed, announce presence via Art-Net Poll Reply
		  ((LXWiFiArtNet*)interface)->send_art_poll_reply(wUDP);
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
    In which case, the data is copied to the dmx_driver object which is driving
    the UART serial DMX output.
  
    If the packet is an ESP-DMX packet, the config struct is modified and stored in EEPROM
  
  if INPUT_TO_NETWORK_MODE:
    if serial dmx has been received, sends an sACN or Art-Net packet containing the dmx data.
    Note:  does not listen for incoming packets for remote configuration in this mode.

*************************************************************************/

void loop() {
  if ( dmx_direction == OUTPUT_FROM_NETWORK_MODE ) {	
	  uint8_t good_dmx = interface->readDMXPacket(wUDP);

	  if ( good_dmx ) {
		  for (int i = 1; i <= interface->numberOfSlots(); i++) {
			  ESP8266DMX.setSlot(i , interface->getSlot(i));
		  }
		  blinkLED();
	  } else {
		 if ( strcmp(ESPDMX_IDENT, (const char *) interface->packetBuffer()) == 0 ) {  //match header to config packet
			Serial.print("ESP-DMX received, ");
			uint8_t reply = 0;
			if ( interface->packetBuffer()[8] == '?' ) {  //packet opcode is query
			  EEPROM.begin(DMXWiFiConfigSIZE);            //deletes data (ie esp_config) and reads from flash
			  esp_config = (DMXWiFiConfig*)EEPROM.getDataPtr();
			  reply = 1;
			} else if (( interface->packetBuffer()[8] == '!' ) && (interface->packetSize() >= 171)) { //packet opcode is set
			  int k;
			  uint8_t* p = (uint8_t*) esp_config;
			  for(k=0; k<171; k++) {
				 p[k] = interface->packetBuffer()[k]; //copy packet to config
			  }
			  esp_config->opcode = 0;               // reply packet opcode is data
			  if (interface->packetSize() >= 203) {
				 for(k=171; k<interface->packetSize(); k++) {
					p[k] = interface->packetBuffer()[k]; //copy rest of packet
				 }
				 strcpy(((LXWiFiArtNet*)interface)->longName(), (char*)&interface->packetBuffer()[171]);
			  }
			  EEPROM.write(8,0);  //opcode is zero for data (also sets dirty flag)
			  EEPROM.commit();
			  reply = 1;
			  Serial.print("eprom written, ");
			} else {
			  Serial.println("packet error.");
			}
			if ( reply) {
			  erasePassword(esp_config);                  //don't show pwd if queried
			  wUDP.beginPacket(wUDP.remoteIP(), interface->dmxPort());
			  wUDP.write((uint8_t*)esp_config, sizeof(*esp_config));
			  wUDP.endPacket();
			  Serial.println("reply complete.");
			}
			interface->packetBuffer()[0] = 0; //insure loop without recv doesn't re-trgger
      blinkLED();
		 } // packet has ESP-DMX header
	  }   // not good_dmx
	  
	} else {		//direction is input to network
	
	  if ( got_dmx ) {
		 interface->setNumberOfSlots(got_dmx);  //got_dmx
     
		 for(int i=1; i<=got_dmx; i++) {
			interface->setSlot(i, ESP8266DMX.getSlot(i));
		 }
     
		 if ( esp_config->protocol_mode & MULTICAST_MODE ) {
			 if (( esp_config->wifi_mode == AP_MODE ) ) {
				 interface->sendDMX(wUDP, esp_config->input_address, WiFi.softAPIP());
			 } else {
				 interface->sendDMX(wUDP, esp_config->input_address, WiFi.localIP());
			 }
		 } else {
			 interface->sendDMX(wUDP, esp_config->input_address, INADDR_NONE);
		 }
		 got_dmx = 0;
		 blinkLED();
	  } // got_dmx
	}   // INPUT_TO_NETWORK_MODE
	
} //loop
