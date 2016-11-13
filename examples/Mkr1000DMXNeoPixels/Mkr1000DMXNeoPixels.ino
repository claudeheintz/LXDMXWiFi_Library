/**************************************************************************/
/*!
    @file     MKR1000DMXNeoPixels.ino
    @author   Claude Heintz
    @license  BSD (see LXDMXWiFi.h)
    @copyright 2016 by Claude Heintz All Rights Reserved

    Example using LXDMXWiFi_Library for output of Art-Net or E1.31 sACN from
    MKR1000 WiFi connection to an Adafruit NeoPixel Ring.
    
    Art-Net(TM) Designed by and Copyright Artistic Licence Holdings Ltd.
    sACN E 1.31 is a public standard published by the PLASA technical standards program
    
    NOTE:  If not using the latest Arduino IDE, you may get compile errors when including the WiFi101 library.
              If you get the following error (and others):
              "error: 'SOCKET' does not name a type SOCKET _socket;"
              Comment out lines 40 and 41 in LXDMXWiFi.h
           
           This example requires the Adafruit NeoPixel Library and WiFi101 Library

           Remote config is supported using the configuration utility iin the examples folder.
           Otherwise edit initConfig() in LXDMXWiFiConfig.cpp.

           As of this version of WiFi101_Library, In access point mode, the MKR1000 does not
           receive broadcast or multicast UDP packets.  So, you must unicast to 192.168.1.1.

    @section  HISTORY

    v1.0 - First release

*/
/**************************************************************************/

#include <WiFi101.h>
#include <WiFiUdp.h>
#include "LXDMXWiFi.h"
#include <LXWiFiArtNet.h>
#include <LXWiFiSACN.h>
#include "LXDMXWiFiConfig.h"

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define STARTUP_MODE_PIN 7      // pin for force default setup when low (use 10k pullup to insure high)
#define LED_PIN 6                // MKR1000 has led on pin 6

#define PIN 5
#define NUM_LEDS 12
Adafruit_NeoPixel ring = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);


const int total_pixels = min(512, 3 * NUM_LEDS);
byte pixels[NUM_LEDS][3];

/*         
 *  Edit the LXDMXWiFiConfig.initConfig() function in LXDMXWiFiConfig.cpp to configure the WiFi connection and protocol options
 */

// dmx protocol interfaces for parsing packets (created in setup)
LXWiFiArtNet* artNetInterface;
LXWiFiSACN*   sACNInterface;

// EthernetUDP instances to let us send and receive UDP packets
WiFiUDP aUDP;
WiFiUDP sUDP;

// direction output from network/input to network
uint8_t dmx_direction = 0;

// Output mode: received packet contained dmx
int art_packet_result = 0;
int acn_packet_result = 0;

/* 
   utility function to toggle indicator LED on/off
*/
uint8_t led_state = 0;

void blinkLED() {
  if ( led_state ) {
    digitalWrite(LED_PIN, HIGH);
    led_state = 0;
  } else {
    digitalWrite(LED_PIN, LOW);
    led_state = 1;
  }
}

/* 
   artAddress callback allows storing of config information
   artAddress may or may not have set this information
   but relevant fields are copied to config struct (and stored to EEPROM ...not yet)
*/
void artAddressReceived() {
  DMXWiFiConfig.setArtNetPortAddress( artNetInterface->universe() );
  DMXWiFiConfig.setNodeName( artNetInterface->longName() );
  DMXWiFiConfig.commitToPersistentStore();
}

/* 
   artIpProg callback allows storing of config information
   cmd field bit 7 indicates that settings should be programmed
*/
void artIpProgReceived(uint8_t cmd, IPAddress addr, IPAddress subnet) {
   if ( cmd & 0x80 ) {
      if ( cmd & 0x40 ) {	//enable dhcp, other fields not written
      	if ( DMXWiFiConfig.staticIPAddress() ) {
      		DMXWiFiConfig.setStaticIPAddress(0);
      	} else {
      	   return;	// already set to dhcp
      	}
      } else {
         if ( ! DMXWiFiConfig.staticIPAddress() ) {
      	   DMXWiFiConfig.setStaticIPAddress(1);	// static not dhcp
      	}
      	if ( cmd & 0x08 ) {	//factory reset
      	   DMXWiFiConfig.initConfig();
      	} else {
      	   if ( cmd & 0x04 ) {	//programIP
      	      DMXWiFiConfig.setStationIPAddress(addr);
      	   }
      	   if ( cmd & 0x02 ) {	//programSubnet
      	      DMXWiFiConfig.setStationSubnetMask(subnet);
      	   }
      	}
      }	// else ( ! dhcp )
      
      DMXWiFiConfig.commitToPersistentStore();
   }
}

/*
  sends pixel buffer to ring
*/

void sendPixels() {
  uint16_t r,g,b;
  for (int p=0; p<NUM_LEDS; p++) {
    r = pixels[p][0];
    g = pixels[p][1];
    b = pixels[p][2];
    r = (r*r)/255;    //gamma correct
    g = (g*g)/255;
    b = (b*b)/255;
    ring.setPixelColor(p, r, g, b);
  }
  ring.show();
}

void setPixel(uint8_t index, uint8_t value) {
  uint8_t pixel = index/3;
  uint8_t color = index%3;
  pixels[pixel][color] = value;
}

/************************************************************************

  Setup creates the WiFi connection.
  
  It also creates the network protocol object,
  either an instance of LXWiFiArtNet or LXWiFiSACN.
  
  if OUTPUT_FROM_NETWORK_MODE:
     Starts listening on the appropriate UDP port.
  
     And, it starts the SAMD21DMX sending serial DMX via the UART1 TX pin.
     (see the SAMD21DMX library documentation for driver details)
     
   if INPUT_TO_NETWORK_MODE:
     Starts SAMD21DMX listening for DMX ( received as serial on UART0 RX pin. )

*************************************************************************/

void setup() {
pinMode(LED_PIN, OUTPUT);
  pinMode(STARTUP_MODE_PIN, INPUT);
  Serial.begin(9600);         //debug messages
  //while ( ! Serial ) {}     //force wait for serial connection.  Sketch will not continue until Serial Monitor is opened.
  Serial.println("_setup_");
  
  uint8_t bootStatus = DMXWiFiConfig.begin(digitalRead(STARTUP_MODE_PIN));
  uint8_t dhcpStatus = 0;

  int wifi_status = WL_IDLE_STATUS;
  if ( DMXWiFiConfig.APMode() ) {                      // WiFi startup
    while (wifi_status == WL_IDLE_STATUS) {            // Access Point mode
      wifi_status = WiFi.beginAP(DMXWiFiConfig.SSID());
      int blink_count = 0;
      while ((wifi_status == WL_IDLE_STATUS) && ( blink_count < 25)) {
        blinkLED();
        blink_count++;
        delay(200);
        wifi_status = WiFi.status();
      }
    }
    if ( DMXWiFiConfig.staticIPAddress() ) {  
      WiFi.config(DMXWiFiConfig.apIPAddress(), (uint32_t)0, DMXWiFiConfig.apGateway(), DMXWiFiConfig.apSubnet());
    }
    Serial.print("Access Point IP Address: ");
  } else {                                             // Station Mode
    while (wifi_status == WL_IDLE_STATUS) {
      wifi_status = WiFi.begin(DMXWiFiConfig.SSID(), DMXWiFiConfig.password());
      int blink_count = 0;
      while ((wifi_status == WL_IDLE_STATUS) && ( blink_count < 25)) {
        blinkLED();
        blink_count++;
        delay(200);
        wifi_status = WiFi.status();
      }
      Serial.print("Station IP Address: ");
    }

    if ( DMXWiFiConfig.staticIPAddress() ) {  
      WiFi.config(DMXWiFiConfig.stationIPAddress(), (uint32_t)0, DMXWiFiConfig.stationGateway(), DMXWiFiConfig.stationSubnet());
      uint8_t dhcpStatus = 0;
    }
     
  }
  
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);
  
  //------------------- Initialize network<->DMX interfaces -------------------
    
  sACNInterface = new LXWiFiSACN();							
  sACNInterface->setUniverse(DMXWiFiConfig.sACNUniverse());

  artNetInterface = new LXWiFiArtNet(WiFi.localIP(), WiFi.subnetMask());
  artNetInterface->setUniverse(DMXWiFiConfig.artnetPortAddress());	//setUniverse for LXArtNet class sets complete Port-Address
  artNetInterface->setArtAddressReceivedCallback(&artAddressReceived);
  artNetInterface->setArtIpProgReceivedCallback(&artIpProgReceived);
  artNetInterface->setStatus2Flag(ARTNET_STATUS2_SACN_CAPABLE, 1);
  artNetInterface->setStatus2Flag(ARTNET_STATUS2_DHCP_CAPABLE, 1);
  if ( dhcpStatus ) {
  	 artNetInterface->setStatus2Flag(ARTNET_STATUS2_DHCP_USED, 1);
  }
  if ( bootStatus ) {
    artNetInterface->setStatus1Flag(ARTNET_STATUS1_FACTORY_BOOT, 1);
  }
  char* nn = DMXWiFiConfig.nodeName();
  if ( nn[0] != 0 ) {
    strcpy(artNetInterface->longName(), nn);
  }
  Serial.print("interfaces created;");
  
  dmx_direction = DMXWiFiConfig.inputToNetworkMode();
  // -------------------
  // if OUTPUT from network, start UDP objects listening for packets
  // and start DMX output
  // -------------------
  if ( dmx_direction == OUTPUT_FROM_NETWORK_MODE ) {  
    if ( ( DMXWiFiConfig.multicastMode() ) ) { // Start listening for UDP on port
      sUDP.beginMulti(DMXWiFiConfig.multicastAddress(), sACNInterface->dmxPort());
    } else {
      sUDP.begin(sACNInterface->dmxPort());
    }
    
    aUDP.begin(artNetInterface->dmxPort());
    artNetInterface->send_art_poll_reply(&aUDP);
    
    Serial.print("udp listening started;");

    ring.begin();
    ring.show();
  } else {                    //direction is INPUT to network
    // doesn't do anything in this mode
  }

  Serial.println("setup complete.");
}

/************************************************************************

  Copy to output merges slots for Art-Net and sACN on HTP basis
  
*************************************************************************/

void copyDMXToOutput(void) {
	uint8_t a, s;
	uint16_t a_slots = artNetInterface->numberOfSlots();
	uint16_t s_slots = sACNInterface->numberOfSlots();
	uint16_t low_addr = DMXWiFiConfig.deviceAddress();
	uint16_t high_addr = DMXWiFiConfig.deviceAddress()+total_pixels;
	for (int i=low_addr; i<high_addr; i++) {
		if ( i <= a_slots ) {
			a = artNetInterface->getSlot(i);
		} else {
			a = 0;
		}
		if ( i <= s_slots ) {
			s = sACNInterface->getSlot(i);
		} else {
			s = 0;
		}
		if ( a > s ) {
			setPixel(i-low_addr, a);
		} else {
			setPixel(i-low_addr, s);
		}
   }
   sendPixels();
}

/************************************************************************

  Checks to see if packet is a config packet.
  
     In the case it is a query, it replies with the current config from persistent storage.
     
     In the case of upload, it copies the payload to persistent storage
     and also replies with the config settings.
  
*************************************************************************/

void checkConfigReceived(LXDMXWiFi* interface, WiFiUDP cUDP) {
	if ( strcmp(CONFIG_PACKET_IDENT, (const char *) interface->packetBuffer()) == 0 ) {	//match header to config packet
		Serial.print("config packet received, ");
		uint8_t reply = 0;
		if ( interface->packetBuffer()[8] == '?' ) {	//packet opcode is query
			DMXWiFiConfig.readFromPersistentStore();
			reply = 1;
		} else if (( interface->packetBuffer()[8] == '!' ) && (interface->packetSize() >= 171)) { //packet opcode is set
			Serial.println("upload packet");
			DMXWiFiConfig.copyConfig( interface->packetBuffer(), interface->packetSize());
			DMXWiFiConfig.commitToPersistentStore();
			reply = 1;
		} else {
			Serial.println("unknown config opcode.");
	  	}
		if ( reply) {
			DMXWiFiConfig.hidePassword();													// don't transmit password!
			cUDP.beginPacket(cUDP.remoteIP(), interface->dmxPort());				// unicast reply
			cUDP.write((uint8_t*)DMXWiFiConfig.config(), DMXWiFiConfigSIZE);
			cUDP.endPacket();
			Serial.println("reply complete.");
			DMXWiFiConfig.restorePassword();
		}
		interface->packetBuffer()[0] = 0; //insure loop without recv doesn't re-trigger
		interface->packetBuffer()[1] = 0;
		blinkLED();
		delay(100);
		blinkLED();
		delay(100);
		blinkLED();
	}		// packet has config packet header
}


/************************************************************************

  Main loop
  
  if OUTPUT_FROM_NETWORK_MODE:
    checks for and reads packets from WiFi UDP socket
    connection.  readDMXPacket() returns true when a DMX packet is received.
    
    If dmx is received on either interface, copy from both (HTP) to dmx output.
  
    If the packet is an CONFIG_PACKET_IDENT packet, the config struct is modified and stored in EEPROM
  
  if INPUT_TO_NETWORK_MODE:
    if serial dmx has been received, sends an sACN or Art-Net packet containing the dmx data.
    Note:  does not listen for incoming packets for remote configuration in this mode.

*************************************************************************/

void loop() {
	if ( dmx_direction == OUTPUT_FROM_NETWORK_MODE ) {
	
		art_packet_result = artNetInterface->readDMXPacket(&aUDP);
		if ( art_packet_result == RESULT_NONE ) {
			checkConfigReceived(artNetInterface, aUDP);
		}
		
		acn_packet_result = sACNInterface->readDMXPacket(&sUDP);
		if ( acn_packet_result == RESULT_NONE ) {
			checkConfigReceived(sACNInterface, sUDP);
		}
		
		if ( (art_packet_result == RESULT_DMX_RECEIVED) || (acn_packet_result == RESULT_DMX_RECEIVED) ) {
			copyDMXToOutput();
			blinkLED();
		}
		
	} else {    // direction is input to network
					// no input mode for this sketch
	}
}// loop()

