/**************************************************************************/
/*!
    @file     ESP-DMX.ino
    @author   Claude Heintz
    @license  BSD (see LXDMXWiFi.h)
    @copyright 2015-2021 by Claude Heintz All Rights Reserved

    Example using LXDMXWiFi_Library for output of Art-Net or E1.31 sACN from
    ESP 8266 WiFi connection to serial DMX.  Or, input from DMX to the network.
    Allows remote configuration of WiFi connection and protocol settings.
    
    Art-Net(TM) Designed by and Copyright Artistic Licence Holdings Ltd.
    sACN E 1.31 is a public standard published by the PLASA technical standards program
    
    Note:  For sending packets larger than 512 bytes, ESP8266 WiFi Library v2.1
           is necessary.
           
           This example requires the LXESP8266DMX library, v2.0 with RDM support
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
    v4.0 - Listens for both Art-Net and sACN in output mode and merges HTP.
    		  Updated LXDMXWiFiConfig and improved remote configuration
    v4.1 - Supports Art-Net 3+ Port Addresses and additional poll reply information

    v5.0 - Adds RDM support (requires LXESP8266DMX library v2.0)
    v5.1 - Change to on demand RDM discovery
    		  RDM discovery runs for limited cycles at startup and after TOD request.
    		  
    v5.2 - Add USE_REMOTE_CONFIG option 

    v6.0 - Refactor to clarify functionality
           -move RDM functions to separate file
           -move Art-Net callbacks and other functions to below setup and loop in main .ino file

    v6.1 - Refactor to move setup of WiFi connection and config packet handling to LXDMXWiFiConfig
*/
/**************************************************************************/

#include "wifi_dmx_rdm.h"

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "LXDMXWiFi.h"
#include <LXWiFiArtNet.h>
#include <LXWiFiSACN.h>
#include <EEPROM.h>
#include "LXDMXWiFiConfig.h"

#define STARTUP_MODE_PIN 16      // pin for force default setup when low (use 10k pullup to insure high)
#define DIRECTION_PIN 4          // pin for output direction enable on MAX481 chip (pin D4 or D15)

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
 *        1) In ESP-DMX.ino line 99 should read:
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

/************************************************************************/
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
  //Serial.begin(115200);
  Serial.setDebugOutput(1); //use uart0 for debugging  UART0
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(STARTUP_MODE_PIN, INPUT_PULLUP);
  pinMode(DIRECTION_PIN, OUTPUT);
  
#ifdef USE_REMOTE_CONFIG
  uint8_t bootStatus = DMXWiFiConfig.begin(digitalRead(STARTUP_MODE_PIN));	// uses settings from persistent memory unless pin is low. 
#else
  uint8_t bootStatus = DMXWiFiConfig.begin();								// must edit DMXWiFiConfig.initConfig() to change settings
#endif
  uint8_t dhcpStatus = 0;
  
  dmx_direction = DMXWiFiConfig.inputToNetworkMode();
  setRDMisEnabled(DMXWiFiConfig.rdmMode());


  if ( rdmIsEnabled() ) {
    if ( dmx_direction ) {
      ESP8266DMX.startRDM(DIRECTION_PIN, 0);  // start RDM in input task mode
    } else {
      ESP8266DMX.startRDM(DIRECTION_PIN);     // defaults to RDM output task mode
    }
  } else if ( dmx_direction ) {					      // DMX Driver startup based on direction flag
    Serial.println("starting DMX input");
    ESP8266DMX.setDirectionPin(DIRECTION_PIN);
    ESP8266DMX.setDataReceivedCallback(&gotDMXCallback);
    ESP8266DMX.startInput();
  } else {
    ESP8266DMX.setDirectionPin(DIRECTION_PIN);
    ESP8266DMX.startOutput();
  }

  // -------------------   WiFi  ------------------- 

  Serial.print("initializing wifi... ");
  
  uint8_t wifi_result = DMXWiFiConfig.setupWiFi(blinkLED);
  
  if ( wifi_result & LX_AP_MODE ) {
    Serial.print("created access point ");
    Serial.println(DMXWiFiConfig.SSID());
  } else {
    if ( ( wifi_result & STATIC_MODE ) == 0 ) {
      dhcpStatus = 1;
    }
    Serial.println("wifi started.");
  }
 
  
  //------------------- Initialize network<->DMX interfaces -------------------
    
  sACNInterface = new LXWiFiSACN();							
  sACNInterface->setUniverse(DMXWiFiConfig.sACNUniverse());

  artNetInterface = new LXWiFiArtNet(WiFi.localIP(), WiFi.subnetMask());
  artNetInterface->setUniverse(DMXWiFiConfig.artnetPortAddress());	//setUniverse for LXArtNet class sets complete Port-Address
  artNetInterface->setArtAddressReceivedCallback(&artAddressReceived);
  artNetInterface->setArtIpProgReceivedCallback(&artIpProgReceived);
  artNetInterface->setArtIndicatorReceivedCallback(&artIndicatorReceived);
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
  if ( rdmIsEnabled() ) {
    artNetInterface->setStatus1Flag(ARTNET_STATUS1_RDM_CAPABLE, 1);
  }
  Serial.print("interfaces created, ");
  
  // start _UDP objects
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
  if ( dmx_direction == 0 ) {  
    artNetInterface->send_art_poll_reply(&aUDP);
  } else {
    artNetInterface->send_art_poll_reply(&aUDP, ARTPOLL_INPUT_MODE);
  }
 Serial.print("udp started listening,");

  Serial.println(" setup complete.");
  blinkLED();
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
	if ( dmx_direction ) {    //direction is input to network
  
		if ( DMXWiFiConfig.sACNMode() ) {
		  checkInput(sACNInterface, &sUDP, DMXWiFiConfig.multicastMode());
		} else {
		  checkInput(artNetInterface, &aUDP, 0);
		  art_packet_result = artNetInterface->readArtNetPacketInputMode(&aUDP);
		  #ifdef USE_REMOTE_CONFIG
		  if ( art_packet_result == RESULT_NONE ) {
			  DMXWiFiConfig.checkConfigReceived(artNetInterface, aUDP, blinkLED, CONFIG_PRINT_MESSAGES);
		  }
      #endif
		}
		
	} else {                  //direction is output from network
  
		art_packet_result = artNetInterface->readDMXPacket(&aUDP);
		#ifdef USE_REMOTE_CONFIG
		if ( art_packet_result == RESULT_NONE ) {
			DMXWiFiConfig.checkConfigReceived(artNetInterface, aUDP, blinkLED, CONFIG_PRINT_MESSAGES);
		}
		#endif
	
		acn_packet_result = sACNInterface->readDMXPacket(&sUDP);
		#ifdef USE_REMOTE_CONFIG
		if ( acn_packet_result == RESULT_NONE ) {
			DMXWiFiConfig.checkConfigReceived(sACNInterface, aUDP, blinkLED, CONFIG_PRINT_MESSAGES);
		}
		#endif
	
		if ( (art_packet_result == RESULT_DMX_RECEIVED) || (acn_packet_result == RESULT_DMX_RECEIVED) ) {
		  copyDMXToOutput();
		  blinkLED();
		  resetRDMIdleCount();
		} else {
		  updateRDM(artNetInterface,aUDP);
		}
		
	}                  //direction is output from network
	
}// loop()

/************************************************************************/
/************************************************************************
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
  DMX input callback function sets number of slots received by ESP8266DMX.
  Input is handled in the main loop by calling checkInput which copies the 
  received values to the network interface if got_dmx is non-zero.
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
      blinkLED();
  }   // <- got_dmx
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
 *          Art-Net Implementation Callback Functions
 ************************************************************************/


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
  uint8_t plen = pdata[1] + 2;
  uint8_t j;

  //copy into ESP8266DMX outgoing packet
  uint8_t* pkt = ESP8266DMX.rdmData();
  for (j=0; j<plen; j++) {
    pkt[j+1] = pdata[j];
  }
  pkt[0] = 0xCC;

  // reply from RDM to network
  if ( ESP8266DMX.sendRDMControllerPacket() ) {
    artNetInterface->send_art_rdm(&aUDP, ESP8266DMX.receivedRDMData(), aUDP.remoteIP());
  }
}

void artCmdReceived(uint8_t* pdata) {
  if ( strcmp((const char*)pdata, "clearSACN") == 0 ) {
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

/*
  Indicator was request by ArtAddress packet
*/
void artIndicatorReceived(bool normal, bool mute, bool locate) {
    if (normal) {
        Serial.println("Normal mode");
    } else if (mute) {
        Serial.println("Mute mode");
    } else if (locate) {
        Serial.println("Locate mode");
    }
}
