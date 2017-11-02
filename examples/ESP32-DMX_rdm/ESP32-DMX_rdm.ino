/**************************************************************************/
/*!
    @file     ESP32-DMX.ino
    @author   Claude Heintz
    @license  BSD (see LXDMXWiFi.h)
    @copyright 2017 by Claude Heintz All Rights Reserved

    Example using LXDMXWiFi_Library for output of Art-Net or E1.31 sACN from
    ESP32 WiFi connection to serial DMX.  Or, input from DMX to the network.
    Allows remote configuration of WiFi connection and protocol settings.
    
    Art-Net(TM) Designed by and Copyright Artistic Licence Holdings Ltd.
    sACN E 1.31 is a public standard published by the PLASA technical standards program
    
    Note:  This example requires the LXESP32DMX library for DMX serial output
           https://github.com/claudeheintz/LXESP32DMX

    @section  HISTORY

    v1.0 - First release

*/
/**************************************************************************/

#include <LXESP32DMX.h>
#include <UID.h>
#include <TOD.h>
#include <rdm_utility.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "LXDMXWiFi.h"
#include <LXWiFiArtNet.h>
#include <LXWiFiSACN.h>
#include "LXDMXWiFiConfig.h"
#include "esp_task_wdt.h"

#define STARTUP_MODE_PIN 18      // pin for force default setup when low (use 10k pullup to insure high)
#define DIRECTION_PIN 21          // pin for output direction enable on MAX481 chip

#define STATUS_LED 19


// RDM defines
#define DISC_STATE_SEARCH 0
#define DISC_STATE_TBL_CK 1
/*
 * If RDM_DISCOVER_ALWAYS == 0, the times RDM discovery runs is limited to 10 cycles
 * of table check and search.  When rdm_discovery_enable reaches zero, continous RDM 
 * discovery stops.  Other ArtRDM packets continue to be relayed.
 * If an Art-Net TODRequest or TODControl packet is received, the rdm_discovery_enable
 * counter is reset and discovery runs again until rdm_discovery_enable reaches zero.
 */
#define RDM_DISCOVER_ALWAYS 0

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
uint8_t rdm_enabled = 0;                      // global RDM flag
uint8_t rdm_discovery_enable = 10;            // limit RDM discovery which can cause flicker in some equipment
uint8_t discovery_state = DISC_STATE_TBL_CK;  // alternates between checking the TOD and discovery search
uint8_t discovery_tbl_ck_index = 0;           // next entry in table to check by sending DISC_MUTE
uint8_t tableChangedFlag = 0;                 // set when TOD is changed by device added or removed
uint8_t idle_count = 0;                       // count to determine cycles devoted to RDM discovery
TOD tableOfDevices;                           // UUIDs of found devices
TOD discoveryTree;                            // stack of UUID ranges for discovery search

UID lower(0,0,0,0,0,0);
UID upper(0,0,0,0,0,0);
UID mid(0,0,0,0,0,0);
UID found(0,0,0,0,0,0);

// used to toggle indicator LED on and off
uint8_t led_state = 0;

/* 
   utility function to toggle indicator LED on/off
*/
void blinkLED() {
  if ( led_state ) {
    digitalWrite(STATUS_LED, HIGH);
    led_state = 0;
  } else {
    digitalWrite(STATUS_LED, LOW);
    led_state = 1;
  }
}

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

void artTodRequestReceived(uint8_t* pdata) {
  rdm_discovery_enable = 10;
  artNetInterface->send_art_tod(&aUDP, tableOfDevices.rawBytes(), tableOfDevices.count());
}

void artRDMReceived(uint8_t* pdata) {
  uint8_t plen = pdata[1] + 2;
  uint8_t j;

  //copy into ESP32DMX outgoing packet
  uint8_t* pkt = ESP32DMX.rdmData();
  for (j=0; j<plen; j++) {
    pkt[j+1] = pdata[j];
  }
  pkt[0] = 0xCC;

    
  if ( ESP32DMX.sendRDMControllerPacket() ) {
    artNetInterface->send_art_rdm(&aUDP, ESP32DMX.receivedRDMData(), aUDP.remoteIP());
  }
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
  DMX input callback function sets number of slots received by ESP32DMX
*/

void gotDMXCallback(int slots) {
  got_dmx = slots;
}

/************************************************************************/

uint8_t testMute(UID u) {
   // try three times to get response when sending a mute message
   if ( ESP32DMX.sendRDMDiscoveryMute(u, RDM_DISC_MUTE) ) {
     return 1;
   }
   if ( ESP32DMX.sendRDMDiscoveryMute(u, RDM_DISC_MUTE) ) {
     return 1;
   }
   if ( ESP32DMX.sendRDMDiscoveryMute(u, RDM_DISC_MUTE) ) {
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
    ESP32DMX.sendRDMDiscoveryMute(BROADCAST_ALL_DEVICES_ID, RDM_DISC_UNMUTE);
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
        uint8_t result = ESP32DMX.sendRDMDiscoveryPacket(lower, upper, &found);
        if ( result ) {
          //this range responded, so divide into sub ranges push them on stack to be further checked
          pushActiveBranch(lower, upper);
           
        } else if ( ESP32DMX.sendRDMDiscoveryPacket(lower, upper, &found) ) {
            pushActiveBranch(lower, upper); //if discovery fails, try a second time
        }
      }         // end check range
      return 1; // UID ranges may be remaining to test
    }           // end valid pop
  }             // end valid pop  
  return 0;     // none left to pop
}

void sendTODifChanged() {
  if ( tableChangedFlag ) {   //if the table has changed...
    tableChangedFlag--;
    
    artNetInterface->send_art_tod(&aUDP, tableOfDevices.rawBytes(), tableOfDevices.count());

    Serial.println("_______________ Table Of Devices _______________");
    tableOfDevices.printTOD();
  }
}

void updateRDMDiscovery() {
  if ( rdm_discovery_enable ) {  // run RDM updates for a limited number of times
    if ( discovery_state ) {
      // check the table of devices
      discovery_tbl_ck_index = checkTable(discovery_tbl_ck_index);
      
      if ( discovery_tbl_ck_index == 0 ) {
        // done with table check
        discovery_state = DISC_STATE_SEARCH;
        pushInitialBranch();
     
        sendTODifChanged();
      } //end table check ended
    } else {    // search for devices in range popped from discoveryTree
  
      if ( checkNextRange() == 0 ) {
        // done with search
        discovery_tbl_ck_index = 0;
        discovery_state = DISC_STATE_TBL_CK;

        sendTODifChanged();
        if ( RDM_DISCOVER_ALWAYS == 0 ) {
          rdm_discovery_enable--;
        }
      }
    }           //end search
  }             // discovery enabled
}
/************************************************************************

  Setup creates the WiFi connection.
  
  It also creates the network protocol object,
  either an instance of LXWiFiArtNet or LXWiFiSACN.
  
  if OUTPUT_FROM_NETWORK_MODE:
     Starts listening on the appropriate UDP port.
  
     And, it starts the ESP32DMX sending serial DMX via the UART1 TX pin.
     (see the LXESP32DMX library documentation for driver details)
     
   if INPUT_TO_NETWORK_MODE:
     Starts ESP32DMX listening for DMX ( received as serial on UART0 RX pin. )

*************************************************************************/

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(1); //use uart0 for debugging
  pinMode(STATUS_LED, OUTPUT);
  pinMode(STARTUP_MODE_PIN, INPUT_PULLUP);
  pinMode(DIRECTION_PIN, OUTPUT);
  //debugging

  
  uint8_t bootStatus = DMXWiFiConfig.begin(1);//aparently need to read from persistent to be able to write to it
  uint8_t dhcpStatus = 0;                     //hence, read in begin and replace below if startup pin is low

  if ( digitalRead(STARTUP_MODE_PIN) == 0 ) {
    DMXWiFiConfig.initConfig();               
  }
  
  dmx_direction = DMXWiFiConfig.inputToNetworkMode();
  rdm_enabled   = DMXWiFiConfig.rdmMode();

  if ( DMXWiFiConfig.APMode() ) {            // WiFi startup
    Serial.print("AP_MODE ");
    Serial.print(DMXWiFiConfig.SSID());
    WiFi.mode(WIFI_AP);
    WiFi.softAP(DMXWiFiConfig.SSID());
    Serial.print("created access point at ");
    Serial.print(DMXWiFiConfig.apIPAddress());
    WiFi.softAPConfig(DMXWiFiConfig.apIPAddress(), DMXWiFiConfig.apGateway(), DMXWiFiConfig.apSubnet());
    
    Serial.print(" accessPoint SSID ");
    Serial.print(DMXWiFiConfig.SSID());
   
    Serial.print(", ");
  } else {
    Serial.print("wifi connecting to ");
    Serial.print(DMXWiFiConfig.SSID());
    Serial.print(" ... ");
    WiFi.mode(WIFI_STA);
    WiFi.begin(DMXWiFiConfig.SSID(),DMXWiFiConfig.password());

    // static IP otherwise uses DHCP
    if ( DMXWiFiConfig.staticIPAddress() ) {  
      WiFi.config(DMXWiFiConfig.stationIPAddress(), DMXWiFiConfig.stationGateway(), DMXWiFiConfig.stationSubnet());
    } else {
      dhcpStatus = 1;
    }

    while (WiFi.status() != WL_CONNECTED)  {
      delay(100);
      blinkLED();
      
    }

  }
  Serial.print("wifi started ");
  Serial.println(WiFi.localIP());

  //------------------- Initialize serialDMX  -------------------

  if ( dmx_direction == OUTPUT_FROM_NETWORK_MODE ) {                // DMX Driver startup based on direction flag
    Serial.println("starting DMX");
    ESP32DMX.startRDM(DIRECTION_PIN);
  } else {
    Serial.println("starting DMX input");
    ESP32DMX.setDirectionPin(DIRECTION_PIN);
    ESP32DMX.setDataReceivedCallback(&gotDMXCallback);
    ESP32DMX.startInput();
  }
  
  //------------------- Initialize network<->DMX interfaces -------------------
    
  sACNInterface = new LXWiFiSACN();							
  sACNInterface->setUniverse(DMXWiFiConfig.sACNUniverse());

  artNetInterface = new LXWiFiArtNet(WiFi.localIP(), WiFi.subnetMask());
  artNetInterface->setUniverse(DMXWiFiConfig.artnetPortAddress());	//setUniverse for LXArtNet class sets complete Port-Address
  artNetInterface->setArtAddressReceivedCallback(&artAddressReceived);
  artNetInterface->setArtIpProgReceivedCallback(&artIpProgReceived);
  artNetInterface->setArtTodRequestCallback(&artTodRequestReceived);
  artNetInterface->setArtRDMCallback(&artRDMReceived);
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
  if ( rdm_enabled ) {
    artNetInterface->setStatus1Flag(ARTNET_STATUS1_RDM_CAPABLE, 1);
  }
  artNetInterface->setStatus1Flag(ARTNET_STATUS1_RDM_CAPABLE, 1);
  Serial.print("interfaces created, ");
  
  // if output from network, start wUDP listening for packets
  if ( dmx_direction == OUTPUT_FROM_NETWORK_MODE ) {	
		if ( DMXWiFiConfig.multicastMode() ) {
			if ( DMXWiFiConfig.APMode() ) {
				sUDP.beginMulticast(DMXWiFiConfig.multicastAddress(), sACNInterface->dmxPort());  //WiFi.softAPIP(), 
			} else {
				sUDP.beginMulticast(DMXWiFiConfig.multicastAddress(), sACNInterface->dmxPort());  //WiFi.localIP(), 
			}
		} else {
			sUDP.begin(sACNInterface->dmxPort());
		}
    
    aUDP.begin(artNetInterface->dmxPort());
    artNetInterface->send_art_poll_reply(&aUDP);
	 Serial.print("udp started listening,");
  }

  Serial.println(" setup complete.");
  blinkLED();

  // trial:  Increase loop priority to 2 above DMX, which is 1
  vTaskPrioritySet(xTaskGetCurrentTaskHandle(), 2);
  
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
      	ESP32DMX.setSlot(i , a);
      } else {
      	ESP32DMX.setSlot(i , s);
      }
   }
   ESP32DMX.setMaxSlots(512);
}

/************************************************************************

  Checks to see if packet is a config packet.
  
     In the case it is a query, it replies with the current config from persistent storage.
     
     In the case of upload, it copies the payload to persistent storage
     and also replies with the config settings.
  
*************************************************************************/

void checkConfigReceived(LXDMXWiFi* interface, WiFiUDP* cUDP) {
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
			ESP.restart();
		} else {
			Serial.println("unknown config opcode.");
	  	}
		if ( reply) {
			DMXWiFiConfig.hidePassword();													// don't transmit password!
			cUDP->beginPacket(cUDP->remoteIP(), interface->dmxPort());				// unicast reply
			cUDP->write((uint8_t*)DMXWiFiConfig.config(), DMXWiFiConfigSIZE);
			cUDP->endPacket();
      Serial.println(DMXWiFiConfig.SSID());
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

  Checks to see if the dmx callback indicates received dmx
     If so, send it using the selected interface.
  
*************************************************************************/

void checkInput(LXDMXWiFi* interface, WiFiUDP* iUDP, uint8_t multicast) {
	if ( got_dmx ) {
		interface->setNumberOfSlots(got_dmx);			// set slots & copy to interface
		for(int i=1; i<=got_dmx; i++) {
		  interface->setSlot(i, ESP32DMX.getSlot(i));
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
			checkConfigReceived(artNetInterface, &aUDP);
		}
    esp_task_wdt_feed();
    vTaskDelay(1);
		
		acn_packet_result = sACNInterface->readDMXPacket(&sUDP);
		if ( acn_packet_result == RESULT_NONE ) {
			checkConfigReceived(sACNInterface, &sUDP);
		}
    esp_task_wdt_feed();
    vTaskDelay(1);
		
		if ( (art_packet_result == RESULT_DMX_RECEIVED) || (acn_packet_result == RESULT_DMX_RECEIVED) ) {
			copyDMXToOutput();
			blinkLED();
		} else {
      // output was not updated last 5 times through loop so use a cycle to perform the next step of RDM discovery
		  if ( rdm_enabled ) {
        idle_count++;
        if ( idle_count > 5 ) {
          updateRDMDiscovery();
          idle_count = 0;
        }
      }
		}
		
	} else {    //direction is input to network
	
		if ( DMXWiFiConfig.sACNMode() ) {
			checkInput(sACNInterface, &sUDP, DMXWiFiConfig.multicastMode());
		} else {
			checkInput(artNetInterface, &aUDP, 0);
		}
		
	}
 
  esp_task_wdt_feed();
  vTaskDelay(1);
  //taskYIELD();
}// loop()

