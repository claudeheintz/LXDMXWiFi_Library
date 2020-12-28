/**************************************************************************/
/*!
    @file     wifi_dmx_rdm.cpp
    @author   Claude Heintz
    @license  BSD (see LXDMXWiFi.h or http://lx.claudeheintzdesign.com/opensource.html)
    @copyright 2016-2021 by Claude Heintz All Rights Reserved
    
    
    Helper Functions to support RDM over Art-Net.
    
*/
/**************************************************************************/

#include "wifi_dmx_rdm.h"

// RDM globals
uint8_t rdm_enabled = 0;            // global RDM flag
uint8_t rdm_discovery_enable = 10;        // limit RDM discovery which can cause flicker in some equipment
uint8_t discovery_state = DISC_STATE_TBL_CK;  // alternates between checking the TOD and discovery search
uint8_t discovery_tbl_ck_index = 0;       // next entry in table to check by sending DISC_MUTE
uint8_t tableChangedFlag = 0;         // set when TOD is changed by device added or removed
uint8_t idle_count = 0;             // count to determine cycles devoted to RDM discovery
TOD tableOfDevices;               // UUIDs of found devices
TOD discoveryTree;                // stack of UUID ranges for discovery search

UID lower(0,0,0,0,0,0);
UID upper(0,0,0,0,0,0);
UID mid(0,0,0,0,0,0);
UID found(0,0,0,0,0,0);

TOD rdmTOD() {
  return tableOfDevices;
}

void setRDMDiscoveryEnable(uint8_t en) {
  rdm_discovery_enable = en;
}

uint8_t rdmIsEnabled() {
  return rdm_enabled;
}

void setRDMisEnabled(uint8_t en) {
  rdm_enabled = en;
}

void resetRDMIdleCount() {
  idle_count = 0;
}

void updateRDM(LXWiFiArtNet* artNetInterface, WiFiUDP aUDP) {
  // output was not updated last 5 times through loop so use a cycle to perform the next step of RDM discovery
  if ( rdm_enabled ) {
    idle_count++;
    if ( idle_count > 5 ) {
      updateRDMDiscovery(artNetInterface, aUDP);
      idle_count = 0;
    }
  }
}

uint8_t testMute(UID u) {
   // try three times to get response when sending a mute message
   if ( ESP8266DMX.sendRDMDiscoveryMute(u, RDM_DISC_MUTE) ) {
     return 1;
   }
   if ( ESP8266DMX.sendRDMDiscoveryMute(u, RDM_DISC_MUTE) ) {
     return 1;
   }
   if ( ESP8266DMX.sendRDMDiscoveryMute(u, RDM_DISC_MUTE) ) {
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
    ESP8266DMX.sendRDMDiscoveryMute(BROADCAST_ALL_DEVICES_ID, RDM_DISC_UNMUTE);
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
        uint8_t result = ESP8266DMX.sendRDMDiscoveryPacket(lower, upper, &found);
        if ( result ) {
          //this range responded, so divide into sub ranges push them on stack to be further checked
          pushActiveBranch(lower, upper);
           
        } else if ( ESP8266DMX.sendRDMDiscoveryPacket(lower, upper, &found) ) {
            pushActiveBranch(lower, upper); //if discovery fails, try a second time
        }
      }         // end check range
      return 1; // UID ranges may be remaining to test
    }           // end valid pop
  }             // end valid pop  
  return 0;     // none left to pop
}

void sendTODifChanged(LXWiFiArtNet* artNetInterface, WiFiUDP aUDP) {
  if ( tableChangedFlag ) {   //if the table has changed...
    tableChangedFlag--;
    
    artNetInterface->send_art_tod(&aUDP, tableOfDevices.rawBytes(), tableOfDevices.count());

#if defined PRINT_DEBUG_MESSAGES
    Serial.println("_______________ Table Of Devices _______________");
    tableOfDevices.printTOD();
#endif
  }
}

void updateRDMDiscovery(LXWiFiArtNet* artNetInterface, WiFiUDP aUDP) {				// RDM discovery replies can cause flicker in some equipment
	if ( rdm_discovery_enable ) {  // run RDM updates for a limited number of times
                          
	  if ( discovery_state ) {
			// check the table of devices
			discovery_tbl_ck_index = checkTable(discovery_tbl_ck_index);
	
			if ( discovery_tbl_ck_index == 0 ) {
			  // done with table check
			  discovery_state = DISC_STATE_SEARCH;
			  pushInitialBranch();
	 
			  sendTODifChanged(artNetInterface, aUDP);
			}                     // <= table check ended
	  } else {                // search for devices in range popped from discoveryTree
  		if ( checkNextRange() == 0 ) {
  		  // done with search
  		  discovery_tbl_ck_index = 0;
  		  discovery_state = DISC_STATE_TBL_CK;
  		 
          sendTODifChanged(artNetInterface, aUDP);
          if ( RDM_DISCOVER_ALWAYS == 0 ) {
            rdm_discovery_enable--;
          }
  		} //  <= if discovery search complete
      }   //  <= discovery search
	}     //  <= rdm_discovery_enable  
}
