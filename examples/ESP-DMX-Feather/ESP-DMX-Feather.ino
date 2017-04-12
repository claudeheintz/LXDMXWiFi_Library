/**************************************************************************/
/*!
    @file     ESP-DMX-Feather.ino
    @author   Claude Heintz
    @license  BSD (see LXDMXWiFi.h)
    @copyright 2017 by Claude Heintz All Rights Reserved

    Example using LXDMXWiFi_Library for output of Art-Net or E1.31 sACN from
    ESP 8266 WiFi connection to serial DMX.  This example is specifically geared to
    Adafruit Feather products.  The Feather ESP8266 Huzzah and the Feather OLED display
    were included in AdaBox 3.  An Adafruit Feather Proto board is added so that a
    MAX485 output driver chip can be fed serial data from the Huzzah pin D2.  Because on
    the OLED board, pin 2 is also wired to a button, the board is modified by cutting
    a trace and jumpering the button to pin 13.
    
    The OLED display shows status, including the board's WiFi SSID, mode and IP address.
    The buttons on the OLED board provide controls for saving and playing a backup scene.
    Configuration of WiFi and Art-Net/sACN Protocols uses the configuration utility which
    is found in the examples for this LXDMXWiFi library.
    
    Art-Net(TM) Designed by and Copyright Artistic Licence Holdings Ltd.
    sACN E 1.31 is a public standard published by the PLASA technical standards program
    
    Note:  For sending packets larger than 512 bytes, ESP8266 WiFi Library v2.1
           is necessary.
           
           This example requires the LXESP8266DMX library for DMX serial output
           https://github.com/claudeheintz/LXESP8266DMX

    @section  HISTORY

    v1.0 - First release
    
*/
/**************************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LXESP8266UARTDMX.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "LXDMXWiFi.h"
#include <LXWiFiArtNet.h>
#include <LXWiFiSACN.h>
#include <EEPROM.h>
#include "LXDMXWiFiConfig.h"

#define DIRECTION_PIN 12    // pin for output direction enable on MAX481 chip
                            // this GPIO is wired to both pins 2 and 3 of the Max481

#define BUTTON_A 0          // buttons on OLED Feather wing
#define BUTTON_B 16         // pin for force default setup when low
#define BUTTON_C 13         // this button re-wired from pin 2 (add 10k external pullup to 3.3v)

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

// Input mode:  received slots when inputting dmx to network
int got_dmx = 0;

// used to toggle indicator LED on and off
uint8_t led_state = 0;

// used to determine if the loop is in run or menu mode (and which menu)
uint8_t menu_mode = 0;
#define MENU_MODE_OFF       0
#define MENU_MODE_PLAYBACK  1
#define MENU_MODE_RECORD    2

// used to toggle on and off playback
uint8_t scene_state = 0;

// *************************** OLED display functions **********************

// the OLED display for the Feather wing
Adafruit_SSD1306 display = Adafruit_SSD1306();

/* 
   utility function for animating display. Indicates DMX input.
*/
void blinkInput() {
  display.fillRect(95,0,30,10, 0);
  display.setCursor(95,0);
  if ( led_state ) {
    display.print("<-DMX");
    led_state = 0;
  } else {
    display.print(" <DMX");
    led_state = 1;
  }
  display.display();
}

/* 
   utility function for animating display. Indicates DMX network packet received.
*/
void blinkOutput() {
  display.fillRect(95,0,30,10, 0);
  display.setCursor(95,0);
  if ( led_state ) {
    display.print("->DMX");
    led_state = 0;
  } else {
    display.print("> DMX");
    led_state = 1;
  }
  display.display();
}

/* 
   utility function for animating display. Indicates connecting.
*/
void blinkConnection() {
  display.fillRect(95,0,30,10, 0);
  display.setCursor(95,0);
  if ( led_state ) {
    display.print("<-->");
    led_state = 0;
  } else {
    display.print(" <> ");
    led_state = 1;
  }
  display.display();
}

/* 
   displays the top line of the main screen
*/

void displayWelcomeLine() {
  display.clearDisplay();
  display.setCursor(0,0);
  /*
  if ( dmx_direction == OUTPUT_FROM_NETWORK_MODE ) {
    display.print("DMX<- ");
  } else {
    display.print("DMX-> ");
  }*/
  display.print(DMXWiFiConfig.SSID());
  display.display();
}

/* 
   displays the second line of the main screen showing Access Point (AP) or Station (IP) and address.
*/
void displayIPLine() {
  display.setCursor(0,12);
  if ( DMXWiFiConfig.APMode() ) {
    display.print("AP: ");
    display.print(WiFi.softAPIP());
  } else {
    display.print("IP: ");
    display.print(WiFi.localIP());
  }
  display.display();
}

/* 
   displays the final line of the main screen .
*/
void displayMenuLine() {
  display.setCursor(0,24);
  display.print("<- Press C for menu.");
  display.display();
}

/* 
   displays the playback on/off line in menu mode
*/
void displayPlayback() {
  display.fillRect(0,0,100,10, 0);
  if ( scene_state ) {  //toggle to off
    display.setCursor(0,0);
    display.print("Cancel Scene");
  } else {
    display.setCursor(0,0);
    display.print("Play Scene");
  }
}

/* 
   displays the first level menu
*/
void displayMenuOne() {
  display.clearDisplay();
  displayPlayback();
  display.setCursor(0,12);
  display.print("Setup");
  display.setCursor(0,24);
  display.print("exit");
  display.display();
}

// *************************** Art-Net functions **********************

/* 
   artAddress callback allows storing of config information
   artAddress may or may not have set this information
   but relevant fields are copied to config struct and stored to EEPROM
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

// *************************** Input function **********************

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
  pinMode(DIRECTION_PIN, OUTPUT);

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT);
  
  uint8_t bootStatus = DMXWiFiConfig.begin(digitalRead(BUTTON_B));
  uint8_t dhcpStatus = 0;
  
  dmx_direction = ( DMXWiFiConfig.inputToNetworkMode() );

  // display setup
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  display.setTextSize(1);
  display.setTextColor(WHITE);
  
  if ( dmx_direction == OUTPUT_FROM_NETWORK_MODE ) {					      // DMX Driver startup based on direction flag
    //ESP8266DMX.setDirectionPin(DIRECTION_PIN);
    ESP8266DMX.startOutput();
  } else {
    //ESP8266DMX.setDirectionPin(DIRECTION_PIN);
    ESP8266DMX.setDataReceivedCallback(&gotDMXCallback);
    ESP8266DMX.startInput();
  }

  displayWelcomeLine();

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
    } else {
      dhcpStatus = 1;
    }
    
    while (WiFi.status() != WL_CONNECTED) {
      delay(100);
      blinkConnection();
    }
  }
  Serial.println("wifi started.");
  displayIPLine();
  
  //------------------- Initialize network<->DMX interfaces -------------------
    
  sACNInterface = new LXWiFiSACN();							
  sACNInterface->setUniverse(DMXWiFiConfig.sACNUniverse());

  if ( DMXWiFiConfig.APMode() ) {
    artNetInterface = new LXWiFiArtNet(WiFi.softAPIP(), WiFi.subnetMask());
  } else {
    artNetInterface = new LXWiFiArtNet(WiFi.localIP(), WiFi.subnetMask());
  }
      
  
  artNetInterface->setUniverse(DMXWiFiConfig.artnetPortAddress());	//setUniverse for LXArtNet class sets complete Port-Address
  artNetInterface->setArtAddressReceivedCallback(&artAddressReceived);
  artNetInterface->setArtIpProgReceivedCallback(&artIpProgReceived);
  char* nn = DMXWiFiConfig.nodeName();
  if ( nn[0] != 0 ) {
    strcpy(artNetInterface->longName(), nn);
  }
  artNetInterface->setStatus2Flag(ARTNET_STATUS2_SACN_CAPABLE, 1);
  artNetInterface->setStatus2Flag(ARTNET_STATUS2_DHCP_CAPABLE, 1);
  if ( dhcpStatus ) {
  	 artNetInterface->setStatus2Flag(ARTNET_STATUS2_DHCP_USED, 1);
  }
  if ( bootStatus ) {
    artNetInterface->setStatus1Flag(ARTNET_STATUS1_FACTORY_BOOT, 1);
  }
  Serial.print("interfaces created, ");
  
  // if output from network, start wUDP listening for packets
  if ( dmx_direction == OUTPUT_FROM_NETWORK_MODE ) {	
		if ( DMXWiFiConfig.multicastMode() ) {
			if ( DMXWiFiConfig.APMode() ) {
				sUDP.beginMulticast(WiFi.softAPIP(), DMXWiFiConfig.multicastAddress(), sACNInterface->dmxPort());
			} else {
				sUDP.beginMulticast(WiFi.localIP(), DMXWiFiConfig.multicastAddress(), sACNInterface->dmxPort());
			}
		} else {
			sUDP.begin(sACNInterface->dmxPort());
		}
    
    aUDP.begin(artNetInterface->dmxPort());
    artNetInterface->send_art_poll_reply(&aUDP);
	 Serial.print("udp started listening,");
  }
  Serial.println(" setup complete.");
  
  displayMenuLine();
} //setup

/************************************************************************

  Copy to output merges slots for Art-Net and sACN on HTP basis
  
*************************************************************************/

void copyDMXToOutput(void) {
	uint8_t a, s;
	uint16_t a_slots = artNetInterface->numberOfSlots();
	uint16_t s_slots = sACNInterface->numberOfSlots();
	for (int i=1; i <=DMX_UNIVERSE_SIZE; i++) {
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
      	ESP8266DMX.setSlot(i , a);
      } else {
      	ESP8266DMX.setSlot(i , s);
      }
   }
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
		} else if ( interface->packetBuffer()[8] == '^' ) {
			ESP.reset();
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
   
		display.fillRect(95,0,30,10, 0);
    display.setCursor(95,0);
    display.print("CONFG");
    display.display();
    delay(1000);
    display.fillRect(95,0,30,10, 0);
    display.display();
	}		// packet has config packet header
}

/************************************************************************

  Checks to see if the dmx callback indicates received dmx
     If so, send it using the selected interface.
  
*************************************************************************/

void checkInput(LXDMXWiFi* interface, WiFiUDP* iUDP, uint8_t multicast) {
	if ( got_dmx ) {
		interface->setNumberOfSlots(got_dmx);			// set slots & copy to interface
		for(int i=1; i<=got_dmx; i++) {
		  interface->setSlot(i, ESP8266DMX.getSlot(i));
		}
		if ( multicast ) {
			interface->sendDMX(iUDP, DMXWiFiConfig.inputAddress(), WiFi.localIP());
		} else {
			interface->sendDMX(iUDP, DMXWiFiConfig.inputAddress(), INADDR_NONE);
		}
      got_dmx = 0;
      blinkInput();
  }       // got_dmx
}

/************************************************************************

  Either writes stored scene to dmx or writes zeros to DMX
  
*************************************************************************/

void cycleScene() {
	if ( scene_state ) {	//toggle to off
		scene_state = 0;
		uint16_t s = ESP8266DMX.numberOfSlots() + 1;
		for(uint16_t i=1; i<s; i++) {
			ESP8266DMX.setSlot(i, 0);
		}
	} else {
		scene_state = 1;
		uint16_t s = DMXWiFiConfig.numberOfSlots() + 1;
    for(uint16_t i=1; i<s; i++) {
			ESP8266DMX.setSlot(i, DMXWiFiConfig.getSlot(i));
		}
	}
  displayPlayback();
  display.display();
}

/************************************************************************

  Remember scene writes the scene to the config structure and stores it
  
*************************************************************************/

void rememberScene() {
	uint16_t s = ESP8266DMX.numberOfSlots();
	DMXWiFiConfig.setNumberOfSlots(s);
	s++;
	for(uint16_t i=1; i<s; i++) {
		DMXWiFiConfig.setSlot(i, ESP8266DMX.getSlot(i));
	}
	DMXWiFiConfig.commitToPersistentStore();
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
  if ( menu_mode == MENU_MODE_OFF ) {
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
  			blinkOutput();
  		}
  		
  	} else {    //direction is input to network
  	
  		if ( DMXWiFiConfig.sACNMode() ) {
  			checkInput(sACNInterface, &sUDP, DMXWiFiConfig.multicastMode());
  		} else {
  			checkInput(artNetInterface, &aUDP, 0);
  		}
  		
  	}
  	if ( digitalRead(BUTTON_C) == LOW ) {
      while ( digitalRead(BUTTON_C) == LOW ) {  //wait for button to be released
        menu_mode = 1;
      }
      displayMenuOne();
  	}
  } else {    // **************** In Menu Mode *********************
  
    if ( digitalRead(BUTTON_C) == LOW ) {         // BUTTON_C always exits menu mode
      if ( menu_mode > 0 ) {
        while ( digitalRead(BUTTON_C) == LOW ) {  //wait for button to be released
          menu_mode = MENU_MODE_OFF;
        }
        displayWelcomeLine();
        displayIPLine();
        displayMenuLine();
      }
    } else {                  // button C high (not pressed) check the other buttons
      
      if ( menu_mode == MENU_MODE_PLAYBACK ) {
        if ( digitalRead(BUTTON_A) == LOW ) {       // button A is playback in this mode
          while ( digitalRead(BUTTON_A) == LOW ) {  // wait for button to be released
          }
          cycleScene();
        }
        if ( digitalRead(BUTTON_B) == LOW ) {         // button B switches to MENU_MODE_RECORD
            while ( digitalRead(BUTTON_B) == LOW ) {  // wait for button to be released
              menu_mode = MENU_MODE_RECORD;
            }
            display.clearDisplay();
            display.setCursor(0,0);
            display.print("Record Scene");
            display.setCursor(0,12);
            display.println("back");
            display.setCursor(0,24);
            display.println("exit");
            display.display();
        }
      
      } else if ( menu_mode == MENU_MODE_RECORD ) {   // check next menu mode
        if ( digitalRead(BUTTON_A) == LOW ) {         // button A is record in this mode
          while ( digitalRead(BUTTON_A) == LOW ) {    // wait for button to be released
          }
          rememberScene();
        }
        if ( digitalRead(BUTTON_B) == LOW ) {         // button B switches to MENU_MODE_PLAYBACK
            while ( digitalRead(BUTTON_B) == LOW ) {  // wait for button to be released
              menu_mode = MENU_MODE_PLAYBACK;
            }
            displayMenuOne();
        }
      }   // == MENU_MODE_RECORD
      
    }     // button C not pressed
  }       // menu mode != MENU_MODE_OFF
	
}// loop()

