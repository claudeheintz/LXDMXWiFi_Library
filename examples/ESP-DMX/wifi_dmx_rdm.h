/**************************************************************************/
/*!
    @file     wifi_dmx_rdm.h
    @author   Claude Heintz
    @license  BSD (see LXDMXWiFi.h or http://lx.claudeheintzdesign.com/opensource.html)
    @copyright 2016-2021 by Claude Heintz All Rights Reserved
    
    
    Helper Functions for implementing RDM over Art-Net.

    Includes device discovery for building a table of devices and sending
    it over Art-Net when requested.
    
*/
/**************************************************************************/

#ifndef wifi_dmx_rdm_h
#define wifi_dmx_rdm_h 1

#include <inttypes.h>
#include <LXESP8266UARTDMX.h>
#include <rdm/rdm_utility.h>
#include <rdm/UID.h>
#include <rdm/TOD.h>
#include "LXDMXWiFi.h"
#include <LXWiFiArtNet.h>


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

TOD rdmTOD();
void setRDMDiscoveryEnable(uint8_t en);
uint8_t rdmIsEnabled();
void setRDMisEnabled(uint8_t en);
void resetRDMIdleCount();
void updateRDM(LXWiFiArtNet* artNetInterface, WiFiUDP aUDP);

uint8_t testMute(UID u);
void checkDeviceFound(UID found);
uint8_t checkTable(uint8_t ck_index);
void pushActiveBranch(UID lower, UID upper);
void pushInitialBranch();
uint8_t checkNextRange();
void sendTODifChanged(LXWiFiArtNet* artNetInterface, WiFiUDP aUDP);
void updateRDMDiscovery(LXWiFiArtNet* artNetInterface, WiFiUDP aUDP);

#endif
