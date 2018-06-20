/**************************************************************************/
/*!
    @file     MKR1000-WiFiDMX.ino
    @author   Claude Heintz
    @license  BSD (see LXDMXWiFi.h)
    @copyright 2016 by Claude Heintz All Rights Reserved

    Example using LXDMXWiFi_Library for output of Art-Net or E1.31 sACN from
    MKR1000 WiFi connection to serial DMX.  Or, input from DMX to the network.
    Allows remote configuration of WiFi connection and protocol settings.
    
    Art-Net(TM) Designed by and Copyright Artistic Licence Holdings Ltd.
    sACN E 1.31 is a public standard published by the PLASA technical standards program
    
    NOTES: If not using the latest Arduino IDE, you may get compile errors when including the WiFi101 library.
              If you get the following error (and others):
              "error: 'SOCKET' does not name a type SOCKET _socket;"
              Comment out lines 40 and 41 in LXDMXWiFi.h
           
           This example requires the LXSAMD21DMX library for DMX serial output
           https://github.com/claudeheintz/LXSAMD21DMX

           Remote config is supported using the configuration
           utility found in the examples folder

           As of this version of WiFi101_Library, In access point mode, the MKR1000 does not
           receive broadcast or multicast UDP packets.  So, you must unicast to 192.168.1.1.

    @section  HISTORY

    v1.0 - First release
    v1.1 - Adds persistence
    v2.0 - Listens on both Art-Net and sACN in output mode and merges HTP.
    		  Updated LXDMXWiFiConfig and improved remote configuration
*/
/**************************************************************************/
#include <LXSAMD21DMX.h>
#include <rdm/UID.h>
#include <rdm/TOD.h>
#include <rdm/rdm_utility.h>
#include <WiFi101.h>
#include <WiFiUdp.h>
#include "LXDMXWiFi.h"
#include <LXWiFiArtNet.h>
#include <LXWiFiSACN.h>
#include "LXDMXWiFiConfig.h"

#define STARTUP_MODE_PIN 7      // pin for force default setup when low (use 10k pullup to insure high)
#define DIRECTION_PIN 3          // pin for output direction enable on MAX481 chip
#define LED_PIN 6                // MKR1000 has led on pin 6

//#define MKR_WIFIDMX_DEBUG_MESSAGES 1

// RDM defines
#define DISC_STATE_SEARCH 0
#define DISC_STATE_TBL_CK 1

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

// RDM globals
uint8_t rdm_enabled = 0;
uint8_t discovery_state = DISC_STATE_TBL_CK;
uint8_t discovery_tbl_ck_index = 0;
uint8_t tableChangedFlag = 0;
uint8_t idle_count = 0;
TOD tableOfDevices;
TOD discoveryTree;

UID lower(0,0,0,0,0,0);
UID upper(0,0,0,0,0,0);
UID mid(0,0,0,0,0,0);
UID found(0,0,0,0,0,0);


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


void artTodRequestReceived(uint8_t* type) {
 #if defined MKR_WIFIDMX_DEBUG_MESSAGES
  Serial.print("artTodRequestReceived");
 #endif
  if ( type[0] ) {
    tableOfDevices.reset();
  }
  artNetInterface->send_art_tod(&aUDP, tableOfDevices.rawBytes(), tableOfDevices.count());
}


void artRDMReceived(uint8_t* pdata) {
  uint8_t plen = pdata[1] + 2;
  uint8_t j;

  //copy into ESP8266DMX outgoing packet
  uint8_t* pkt = SAMD21DMX.rdmData();
  for (j=0; j<plen; j++) {
    pkt[j+1] = pdata[j];
  }
  pkt[0] = 0xCC;

    
  if ( SAMD21DMX.sendRDMControllerPacket() ) {
    artNetInterface->send_art_rdm(&aUDP, SAMD21DMX.receivedRDMData(), aUDP.remoteIP());
  }
}

/*
  DMX input callback function sets number of slots received by SAMD21DMX
*/
void gotDMXCallback(int slots) {
  got_dmx = slots;
}

/************************************************************************/

uint8_t testMute(UID u) {
   // try three times to get response when sending a mute message
   if ( SAMD21DMX.sendRDMDiscoveryMute(u, RDM_DISC_MUTE) ) {
     return 1;
   }
   if ( SAMD21DMX.sendRDMDiscoveryMute(u, RDM_DISC_MUTE) ) {
     return 1;
   }
   if ( SAMD21DMX.sendRDMDiscoveryMute(u, RDM_DISC_MUTE) ) {
     return 1;
   }
   return 0;
}

void checkDeviceFound(UID found) {
  if ( testMute(found) ) {
    tableOfDevices.add(found);
    tableChangedFlag = 1;
  }
}

uint8_t checkTable(uint8_t ck_index) {
  if ( ck_index == 0 ) {
    SAMD21DMX.sendRDMDiscoveryMute(BROADCAST_ALL_DEVICES_ID, RDM_DISC_UNMUTE);
  }

  if ( tableOfDevices.getUIDAt(ck_index, &found) )  {
    if ( testMute(found) ) {
      // device confirmed
      return ck_index += 6;
    }
    
    // device not found
    tableOfDevices.removeUIDAt(ck_index);
    tableChangedFlag = 1;
    return ck_index;
  }
  // index invalid
  return 0;
}

//called when range responded, so divide into sub ranges push them on stack to be further checked
void pushActiveBranch(UID lower, UID upper) {
  if ( mid.becomeMidpoint(lower, upper) ) {
    discoveryTree.push(lower);
    discoveryTree.push(mid);
    discoveryTree.push(mid);
    discoveryTree.push(upper);
  } else {
    // No midpoint possible:  lower and upper are equal or a 1 apart
    checkDeviceFound(lower);
    checkDeviceFound(upper);
  }
}

void pushInitialBranch() {
  lower.setBytes(0);
  upper.setBytes(BROADCAST_ALL_DEVICES_ID);
  discoveryTree.push(lower);
  discoveryTree.push(upper);

  //ETC devices seem to only respond with wildcard or exact manufacturer ID
  lower.setBytes(0x657400000000);
  upper.setBytes(0x6574FFFFFFFF);
  discoveryTree.push(lower);
  discoveryTree.push(upper);
}

uint8_t checkNextRange() {
  if ( discoveryTree.pop(&upper) ) {
    if ( discoveryTree.pop(&lower) ) {
      if ( lower == upper ) {
        checkDeviceFound(lower);
      } else {        //not leaf so, check range lower->upper
        uint8_t result = SAMD21DMX.sendRDMDiscoveryPacket(lower, upper, &found);
        if ( result ) {
          //this range responded, so divide into sub ranges push them on stack to be further checked
          pushActiveBranch(lower, upper);
           
        } else if ( SAMD21DMX.sendRDMDiscoveryPacket(lower, upper, &found) ) {
            pushActiveBranch(lower, upper); //if discovery fails, try a second time
        }
      }         // end check range
      return 1; // UID ranges may be remaining to test
    }           // end valid pop
  }             // end valid pop  
  return 0;     // none left to pop
}

void updateRDMDiscovery() {
  if ( discovery_state ) {
    // check the table of devices
    discovery_tbl_ck_index = checkTable(discovery_tbl_ck_index);
    
    if ( discovery_tbl_ck_index == 0 ) {
      // done with table check
      discovery_state = DISC_STATE_SEARCH;
      pushInitialBranch();
   
      if ( tableChangedFlag ) {   //if the table has changed...
        tableChangedFlag = 0;

        artNetInterface->send_art_tod(&aUDP, tableOfDevices.rawBytes(), tableOfDevices.count());
        // if this were an Art-Net application, you would send an 
        // ArtTOD packet here, because the device table has changed.
        // for this test, we just print the list of devices
 #if defined MKR_WIFIDMX_DEBUG_MESSAGES
        Serial.println("_______________ Table Of Devices _______________");
        tableOfDevices.printTOD();
 #endif
      }
    } //end table check ended
  } else {    // search for devices in range popped from discoveryTree

    if ( checkNextRange() == 0 ) {
      // done with search
      discovery_tbl_ck_index = 0;
      discovery_state = DISC_STATE_TBL_CK;
    }
  }           //end search
}




/************************************************************************************************************************************************
 ************************************************************************************************************************************************

  Setup creates the WiFi connection.
  
  It also creates the network protocol object,
  either an instance of LXWiFiArtNet or LXWiFiSACN.
  
  if OUTPUT_FROM_NETWORK_MODE:
     Starts listening on the appropriate UDP port.
  
     And, it starts the SAMD21DMX sending serial DMX via the UART1 TX pin.
     (see the SAMD21DMX library documentation for driver details)
     
   if INPUT_TO_NETWORK_MODE:
     Starts SAMD21DMX listening for DMX ( received as serial on UART0 RX pin. )

*************************************************************************************************************************************************
*************************************************************************************************************************************************/

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(STARTUP_MODE_PIN, INPUT_PULLUP);

#if defined MKR_WIFIDMX_DEBUG_MESSAGES
  Serial.begin(115200);         //debug messages
  while ( ! Serial ) {}     //force wait for serial connection.  Sketch will not continue until Serial Monitor is opened.
#endif

  uint8_t bootStatus = DMXWiFiConfig.begin(digitalRead(STARTUP_MODE_PIN));
  uint8_t dhcpStatus = 0;
  dmx_direction = DMXWiFiConfig.inputToNetworkMode();
  rdm_enabled   = DMXWiFiConfig.rdmMode();

#if defined MKR_WIFIDMX_DEBUG_MESSAGES
  Serial.print("Config begin with SSID-> ");
  Serial.println(DMXWiFiConfig.SSID());
#endif

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
#if defined MKR_WIFIDMX_DEBUG_MESSAGES
    Serial.print("Access Point IP Address: ");
#endif
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
#if defined MKR_WIFIDMX_DEBUG_MESSAGES
      Serial.print("Station IP Address: ");
#endif
    }

    if ( DMXWiFiConfig.staticIPAddress() ) {  
      WiFi.config(DMXWiFiConfig.stationIPAddress(), (uint32_t)0, DMXWiFiConfig.stationGateway(), DMXWiFiConfig.stationSubnet());
      uint8_t dhcpStatus = 0;
    }
     
  }

#if defined MKR_WIFIDMX_DEBUG_MESSAGES
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);
#endif
  
  //------------------- Initialize network<->DMX interfaces -------------------
    
  sACNInterface = new LXWiFiSACN();							
  sACNInterface->setUniverse(DMXWiFiConfig.sACNUniverse());

  artNetInterface = new LXWiFiArtNet(WiFi.localIP(), WiFi.subnetMask());
  artNetInterface->setUniverse(DMXWiFiConfig.artnetPortAddress());	//setUniverse for LXArtNet class sets complete Port-Address
  artNetInterface->setArtAddressReceivedCallback(&artAddressReceived);
  artNetInterface->setArtIpProgReceivedCallback(&artIpProgReceived);
  if ( rdm_enabled ) {
    artNetInterface->setArtTodRequestCallback(&artTodRequestReceived);
    artNetInterface->setArtRDMCallback(&artRDMReceived);
  }
  artNetInterface->setStatus2Flag(ARTNET_STATUS2_SACN_CAPABLE, 1);
  artNetInterface->setStatus2Flag(ARTNET_STATUS2_DHCP_CAPABLE, 1);
  if ( dhcpStatus ) {
  	 artNetInterface->setStatus2Flag(ARTNET_STATUS2_DHCP_USED, 1);
  }
  if ( bootStatus ) {
    artNetInterface->setStatus1Flag(ARTNET_STATUS1_FACTORY_BOOT, 1);
  }
  if ( rdm_enabled ) {
    artNetInterface->setStatus1Flag(ARTNET_STATUS1_RDM_CAPABLE, 1);
  }
  char* nn = DMXWiFiConfig.nodeName();
  if ( nn[0] != 0 ) {
    strcpy(artNetInterface->longName(), nn);
  }
#if defined MKR_WIFIDMX_DEBUG_MESSAGES
  Serial.print("interfaces created;");
#endif
  
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
    if ( dmx_direction == OUTPUT_FROM_NETWORK_MODE ) {
    	artNetInterface->send_art_poll_reply(&aUDP);
    } else {
    	artNetInterface->send_art_poll_reply(&aUDP, ARTPOLL_INPUT_MODE);
    }

#if defined MKR_WIFIDMX_DEBUG_MESSAGES
    Serial.print("udp listening started-starting DMX output;");
#endif

    if ( rdm_enabled ) {
 #if defined MKR_WIFIDMX_DEBUG_MESSAGES
    Serial.print("RDM enabled;");
 #endif
      SAMD21DMX.startRDM(DIRECTION_PIN);
    } else {
      SAMD21DMX.setDirectionPin(DIRECTION_PIN);
      SAMD21DMX.startOutput();
    }
  } else {             //-------------------direction is INPUT to network, start DMX input
#if defined MKR_WIFIDMX_DEBUG_MESSAGES
    Serial.print("starting DMX input;");
#endif
    SAMD21DMX.setDirectionPin(DIRECTION_PIN);
    SAMD21DMX.setDataReceivedCallback(&gotDMXCallback);
    SAMD21DMX.startInput();
  }

#if defined MKR_WIFIDMX_DEBUG_MESSAGES
  Serial.println("setup complete.");
#endif

} //setup()

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
      	SAMD21DMX.setSlot(i , a);
      } else {
      	SAMD21DMX.setSlot(i , s);
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
#if defined MKR_WIFIDMX_DEBUG_MESSAGES
		Serial.print("config packet received, ");
   Serial.print(interface->packetSize());
   Serial.print(" bytes,");
#endif
		uint8_t reply = 0;
		if ( interface->packetBuffer()[8] == '?' ) {	//packet opcode is query
			DMXWiFiConfig.readFromPersistentStore();
			reply = 1;
		} else if (( interface->packetBuffer()[8] == '!' ) && (interface->packetSize() >= 171)) { //packet opcode is set
#if defined MKR_WIFIDMX_DEBUG_MESSAGES
    Serial.print("upload packet, ");
#endif
			DMXWiFiConfig.copyConfig( interface->packetBuffer(), interface->packetSize());
      SAMD21DMX.stop();   //conflicts (interrupts with flash erase)  requires device reset after programming
      delay(500);
			DMXWiFiConfig.commitToPersistentStore();
#if defined MKR_WIFIDMX_DEBUG_MESSAGES
      Serial.println("written to flash");
#endif
			reply = 1;
		} else {
#if defined MKR_WIFIDMX_DEBUG_MESSAGES
			Serial.println("unknown config opcode.");
#endif
	  	}
		if ( reply) {
			DMXWiFiConfig.hidePassword();													// don't transmit password!
			cUDP.beginPacket(cUDP.remoteIP(), interface->dmxPort());				// unicast reply
			cUDP.write((uint8_t*)DMXWiFiConfig.config(), DMXWiFiConfigSIZE);
			cUDP.endPacket();
#if defined MKR_WIFIDMX_DEBUG_MESSAGES
			Serial.println("reply complete.");
			DMXWiFiConfig.restorePassword();
#endif
		}
		interface->packetBuffer()[0] = 0; //insure loop without recv doesn't re-trigger
		interface->packetBuffer()[1] = 0;
		blinkLED();
		delay(100);
		blinkLED();
		delay(100);
		blinkLED();
#if defined MKR_WIFIDMX_DEBUG_MESSAGES
    Serial.print("config complete ssid->");
  Serial.println(DMXWiFiConfig.SSID());
#endif
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
		  interface->setSlot(i, SAMD21DMX.getSlot(i));
		}
		if ( multicast ) {
			interface->sendDMX(iUDP, DMXWiFiConfig.inputAddress(), WiFi.localIP());
		} else {
			interface->sendDMX(iUDP, DMXWiFiConfig.inputAddress(), INADDR_NONE);
		}
      got_dmx = 0;
      blinkLED();
    }       // got_dmx
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
		} else if ( rdm_enabled ) {
      updateRDMDiscovery();
		}
		
	} else {    //direction is input to network
	
		if ( DMXWiFiConfig.sACNMode() ) {
			checkInput(sACNInterface, &sUDP, DMXWiFiConfig.multicastMode());
		} else {
			checkInput(artNetInterface, &aUDP, 0);
			art_packet_result = artNetInterface->readArtNetPacketInputMode(&aUDP);
			if ( art_packet_result == RESULT_NONE ) {
				checkConfigReceived(artNetInterface, aUDP);
			}
		}
		
	}
}// loop()
