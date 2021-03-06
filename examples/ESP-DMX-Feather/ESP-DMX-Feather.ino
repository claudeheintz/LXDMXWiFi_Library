/**************************************************************************/
/*!
    @file     ESP-DMX-Feather.ino
    @author   Claude Heintz
    @license  BSD (see LXDMXWiFi.h)
    @copyright 2017-2018 by Claude Heintz All Rights Reserved

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
           (v2.0 of LXESP8266DMX library required for RDM)

    @section  HISTORY

    v1.0   - First release
    v2.0   - Updated for OLED Feather Wing and RDM
    v2.0.1 - Scene record triggered by ArtCommand
    v2.0.2 - Add USE_REMOTE_CONFIG option
    v3.0   - Refactor to clarify functionality
             -move RDM functions to separate file
             -move Art-Net callbacks and other functions to below setup and loop in main .ino file
*/
/**************************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "wifi_dmx_rdm.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "LXDMXWiFi.h"
#include <LXWiFiArtNet.h>
#include <LXWiFiSACN.h>
#include <EEPROM.h>
#include "LXDMXWiFiConfig.h"

//#define ESP_PRINT_DEBUG_MSGS 1

#define DIRECTION_PIN 15    // pin for output direction enable on MAX481 chip
                            // this GPIO is wired to both pins 2 and 3 of the Max481

#define RDM_CAPABLE_HARDWARE 1

//#define LED_PIN_INPUT 14  // indicator LED(s)
#define LED_PIN_OUTPUT 14


#define BUTTON_A 0          // buttons on OLED Feather wing
#define BUTTON_B 16         // pin for force default setup when low
#define BUTTON_C 13         // this button re-wired from pin 2 (add 10k external pullup to 3.3v)

/*         
 *  To allow use of the configuration utility, uncomment the following statement
 *  to define USE_REMOTE_CONFIG.
 *
 *  When using remote configuration:
 *        The remote configuration utility can be used to edit the settings
 *        without re-loading the sketch.  Settings from persistent memory are
 *        used unless the startup pin is read LOW.  Holding the startup pin low
 *        temporarily uses the settings in the LXDMXWiFiConfig.initConfig() method.
 *        This insures there is a default way of connecting to the sketch
 *        in order to use the remote utility, even if it is configured to use
 *        a WiFi network that is unavailable.
 *
 *	Without remote configuration (USE_REMOTE_CONFIG remains undefined):
 *        Settings are read from the LXDMXWiFiConfig.initConfig() method.
 *        So, it is necessary to edit that function in order to change
 *        the settings.
 *
 *  Important for using remote configuration:
 *        It is necessary to initialize the boards EEPROM flash with the
 *        following steps.
 *
 *       ------------------------------------------------
 *        1) In ESP-DMX.ino line 83 should read:
 *           #define USE_REMOTE_CONFIG 0
 *
 *        2) In LXDMXWiFiConfig.cpp uncomment line 35:
 *           #define RESET_PERSISTENT_CONFIG_ON_DEFAULT 1
 *
 *        3) Flash the ESP8266 and reboot.
 *
 *        4) In LXDMXWiFiConfig.cpp comment out line 35:
 *           //#define RESET_PERSISTENT_CONFIG_ON_DEFAULT 1
 *
 *        5) Flash the ESP8266 and reboot.
*       --------------------------------------------------
 */
#define USE_REMOTE_CONFIG 0

uint8_t print_config_messages = 0;

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



// the OLED display for the Feather wing
Adafruit_SSD1306 display = Adafruit_SSD1306();

/* some of the functions defined below loop
void artAddressReceived();
void artIpProgReceived(uint8_t cmd, IPAddress addr, IPAddress subnet);
void artTodRequestReceived(uint8_t* type);
void artRDMReceived(uint8_t* pdata);
void artCmdReceived(uint8_t* pdata);
void cycleScene();
*/

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
#if defined ESP_PRINT_DEBUG_MSGS
  Serial.begin(115200);
#endif
  Serial.setDebugOutput(UART0); //use uart0 for debugging
 #if defined DIRECTION_PIN
  pinMode(DIRECTION_PIN, OUTPUT);
 #endif
 #if defined LED_PIN_OUTPUT
  pinMode(LED_PIN_OUTPUT, OUTPUT);
 #endif
  #if defined LED_PIN_INPUT
  pinMode(LED_PIN_INPUT, OUTPUT);
 #endif

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT);
  
// -------------------   initialize config  ------------------- 
#ifdef USE_REMOTE_CONFIG
  uint8_t bootStatus = DMXWiFiConfig.begin(digitalRead(BUTTON_B));			// uses settings from persistent memory unless pin is low.
#else
  uint8_t bootStatus = DMXWiFiConfig.begin();								// must edit DMXWiFiConfig.initConfig() to change settings
#endif
  uint8_t dhcpStatus = 0;
  
  dmx_direction = ( DMXWiFiConfig.inputToNetworkMode() );
#if defined RDM_CAPABLE_HARDWARE
  setRDMisEnabled(DMXWiFiConfig.rdmMode());
  if ( digitalRead(BUTTON_C) == 0 ) {
    setRDMisEnabled( ! rdmIsEnabled() );
  }
#endif

// -------------------   display setup  ------------------- 
// by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  display.setTextSize(1);
  display.setTextColor(WHITE);
  
// -------------------   DMX  ------------------- 
  if ( rdmIsEnabled() ) {
    if ( dmx_direction ) {
      ESP8266DMX.startRDM(DIRECTION_PIN, 0);  // start RDM in input task mode
    } else {
      ESP8266DMX.startRDM(DIRECTION_PIN);     // defaults to RDM output task mode
    }
    
    LX8266DMX::THIS_DEVICE_ID.setBytes(0x6C, 0x78, 0x01, 0x02, 0x03, 0x04);		//change device ID from default
    
  } else if ( dmx_direction ) {					      // DMX Driver startup based on direction flag
    #if defined DIRECTION_PIN
      ESP8266DMX.setDirectionPin(DIRECTION_PIN);
    #endif
    ESP8266DMX.setDataReceivedCallback(&gotDMXCallback);
    ESP8266DMX.startInput();
  } else {
    #if defined DIRECTION_PIN
      ESP8266DMX.setDirectionPin(DIRECTION_PIN);
    #endif
    ESP8266DMX.startOutput();
  }

  displayWelcomeLine();

// -------------------   WiFi  ------------------- 

#if defined ESP_PRINT_DEBUG_MSGS
    Serial.print("initializing wifi... ");
    print_config_messages = 1;
#endif

  uint8_t wifi_result = DMXWiFiConfig.setupWiFi(blinkConnection);
  
  if ( wifi_result & LX_AP_MODE ) {
 #if defined ESP_PRINT_DEBUG_MSGS
    Serial.print("created access point ");
    Serial.println(DMXWiFiConfig.SSID());
    Serial.print(", ");
#endif
  } else {
    if ( ( wifi_result & STATIC_MODE ) == 0 ) {
      dhcpStatus = 1;
    }
#if defined ESP_PRINT_DEBUG_MSGS
  Serial.println("wifi started.");
#endif
  }
 

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
  if ( rdmIsEnabled() ) {
    artNetInterface->setArtTodRequestCallback(&artTodRequestReceived);
    artNetInterface->setArtRDMCallback(&artRDMReceived);
  }
  artNetInterface->setArtCommandCallback(&artCmdReceived);
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
#if defined ESP_PRINT_DEBUG_MSGS
  Serial.print("interfaces created, ");
#endif
 
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
  if ( dmx_direction ) {
    artNetInterface->send_art_poll_reply(&aUDP, ARTPOLL_INPUT_MODE);
  } else {
    artNetInterface->send_art_poll_reply(&aUDP);
  }
  
#if defined ESP_PRINT_DEBUG_MSGS
  Serial.print("udp started listening,");
  Serial.println(" setup complete.");
#endif
  
  displayMenuLine();
} //setup

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
    
  	if ( dmx_direction ) {    //direction is input to network
    
      if ( DMXWiFiConfig.sACNMode() ) {
        checkInput(sACNInterface, &sUDP, DMXWiFiConfig.multicastMode());
      } else {
        checkInput(artNetInterface, &aUDP, 0);
        art_packet_result = artNetInterface->readArtNetPacketInputMode(&aUDP);
		#ifdef USE_REMOTE_CONFIG
		  if ( art_packet_result == RESULT_NONE ) {
			  checkConfigReceived(artNetInterface, aUDP);
		  }
      	#endif
      }
      
    } else {    //direction is output to network
  	
  		art_packet_result = artNetInterface->readDMXPacket(&aUDP);
		#ifdef USE_REMOTE_CONFIG
		if ( art_packet_result == RESULT_NONE ) {
			checkConfigReceived(artNetInterface, aUDP);
		}
		#endif
  		
  		acn_packet_result = sACNInterface->readDMXPacket(&sUDP);
		#ifdef USE_REMOTE_CONFIG
		if ( acn_packet_result == RESULT_NONE ) {
			checkConfigReceived(sACNInterface, sUDP);
		}
		#endif
  		
		if ( (art_packet_result == RESULT_DMX_RECEIVED) || (acn_packet_result == RESULT_DMX_RECEIVED) ) {
			copyDMXToOutput();
			blinkOutput();
			resetRDMIdleCount();
		} else {
      updateRDM(artNetInterface,aUDP);
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
                        		// button C high (not pressed) check the another button
    } else if ( menu_mode == MENU_MODE_PLAYBACK ) {
        if ( digitalRead(BUTTON_A) == LOW ) {       // button A is playback in this mode
          while ( digitalRead(BUTTON_A) == LOW ) {  // wait for button to be released
          }
          cycleScene();
        } // button a pressed
      }	  // playback mode, button C not pressed
    }     // menu mode != MENU_MODE_OFF
	
}// loop()


// *************************** OLED display functions **********************

/* 
   Indicates DMX input.
*/
void blinkInput() {
 #if defined LED_PIN_INPUT
  if ( led_state ) {
    digitalWrite(LED_PIN_INPUT, HIGH);
    led_state = 0;
  } else {
    digitalWrite(LED_PIN_INPUT, LOW);
    led_state = 1;
  }
 #endif
}

/* 
   Indicates DMX network packet received.
*/
void blinkOutput() {
  #if defined LED_PIN_OUTPUT
  if ( led_state ) {
    digitalWrite(LED_PIN_OUTPUT, HIGH);
    led_state = 0;
  } else {
    digitalWrite(LED_PIN_OUTPUT, LOW);
    led_state = 1;
  }
  #endif
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
  if ( rdmIsEnabled() ) {
    display.print("_");
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
  display.setCursor(0,24);
  display.print("exit");
  display.display();
}

// *************************** Input function **********************

/*
  DMX input callback function sets number of slots received by ESP8266DMX
*/

void gotDMXCallback(int slots) {
  got_dmx = slots;
}

/************************************************************************

  Checks to see if the dmx callback indicates received dmx
     If so, send it using the selected interface.
  
*************************************************************************/

void checkInput(LXDMXWiFi* interface, WiFiUDP* iUDP, uint8_t multicast) {
  if ( got_dmx ) {
    interface->setNumberOfSlots(got_dmx);     // set slots & copy to interface
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
 
  
*************************************************************************/

void checkConfigReceived(LXDMXWiFi* interface, WiFiUDP cUDP) {
  if ( DMXWiFiConfig.checkConfigReceived(interface, cUDP, blinkConnection, print_config_messages ) ) {
    display.fillRect(95,0,30,10, 0);
    display.setCursor(95,0);
    display.print("CONFG");
    display.display();
    delay(1000);
    display.fillRect(95,0,30,10, 0);
    display.display();
  }   // packet has config packet header
}



/************************************************************************

  Either writes stored scene to dmx or writes zeros to DMX
  
*************************************************************************/

void cycleScene() {
  if ( scene_state ) {  //toggle to off
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

void artTodRequestReceived(uint8_t* type) {
  if ( type[0] ) {
    rdmTOD().reset();
  }
  setRDMDiscoveryEnable(10);
  artNetInterface->send_art_tod(&aUDP, rdmTOD().rawBytes(), rdmTOD().count());
}

void artRDMReceived(uint8_t* pdata) {
#if defined ESP_PRINT_DEBUG_MSGS
  Serial.println("got rdm!");
#endif
  uint8_t plen = pdata[1] + 2;
  uint8_t j;

  //copy into ESP8266DMX outgoing packet
  uint8_t* pkt = ESP8266DMX.rdmData();
  for (j=0; j<plen; j++) {
    pkt[j+1] = pdata[j];
  }
  pkt[0] = 0xCC;

    
  if ( ESP8266DMX.sendRDMControllerPacket() ) {
#if defined ESP_PRINT_DEBUG_MSGS
    Serial.println("sent and got response!");
#endif
    artNetInterface->send_art_rdm(&aUDP, ESP8266DMX.receivedRDMData(), aUDP.remoteIP());
  }
}

void artCmdReceived(uint8_t* pdata) {
  if ( strcmp((const char*)pdata, "record=1") == 0 ) {
    rememberScene();
    for (int j=0; j<3; j++) {
      blinkConnection();
      delay(100);
      blinkConnection();
      delay(100);
    }
  } else if ( strcmp((const char*)pdata, "clearSACN") == 0 ) {
    sACNInterface->clearDMXOutput();
  }
}

/* 
   artIpProg callback allows storing of config information
   cmd field bit 7 indicates that settings should be programmed
*/
void artIpProgReceived(uint8_t cmd, IPAddress addr, IPAddress subnet) {
   if ( cmd & 0x80 ) {
      if ( cmd & 0x40 ) { //enable dhcp, other fields not written
        if ( DMXWiFiConfig.staticIPAddress() ) {
          DMXWiFiConfig.setStaticIPAddress(0);
        } else {
           return;  // already set to dhcp
        }
      } else {
         if ( ! DMXWiFiConfig.staticIPAddress() ) {
           DMXWiFiConfig.setStaticIPAddress(1); // static not dhcp
        }
        if ( cmd & 0x08 ) { //factory reset
           DMXWiFiConfig.initConfig();
        } else {
           if ( cmd & 0x04 ) {  //programIP
              DMXWiFiConfig.setStationIPAddress(addr);
           }
           if ( cmd & 0x02 ) {  //programSubnet
              DMXWiFiConfig.setStationSubnetMask(subnet);
           }
        }
      } // else ( ! dhcp )
      
      DMXWiFiConfig.commitToPersistentStore();
   }
}
