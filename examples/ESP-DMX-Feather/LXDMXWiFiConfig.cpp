/**************************************************************************/
/*!
    @file     LXDMXWiFiconfig.cpp
    @author   Claude Heintz
    @license  BSD (see LXDMXWiFi.h or http://lx.claudeheintzdesign.com/opensource.html)
    @copyright 2016 by Claude Heintz All Rights Reserved

    Edit initConfig for your own default settings.
*/
/**************************************************************************/

#include <Arduino.h>
#include <IPAddress.h>
#include <EEPROM.h>
#include "LXDMXWiFiConfig.h"


DMXwifiConfig DMXWiFiConfig;

/*******************************************************************************
 ***********************  DMXwifiConfig member functions  ********************/

DMXwifiConfig::DMXwifiConfig ( void ) {
	
}

DMXwifiConfig::~DMXwifiConfig ( void ) {
	if ( _temp_config ) {
		free(_wifi_config);
	}
}

uint8_t DMXwifiConfig::begin ( uint8_t mode ) {
	if ( mode ) {
		_temp_config = 0;
		EEPROM.begin(DMXWiFiConfigSIZE);                      		// get pointer to data
  		_wifi_config = (DMXWiFiconfig*)EEPROM.getDataPtr();
  	
		// data already read, check initialization
		if (( strcmp(CONFIG_PACKET_IDENT, (const char *) _wifi_config) != 0 ) ||
			( _wifi_config->version > DMXWIFI_CONFIG_INVALID_VERSION )) {	          // if structure not previously stored or invalid version
		  initConfig();																  // initialize and store in EEPROM
		  commitToPersistentStore();
		  Serial.println("\nInitialized EEPROM");
		} else {																	  // otherwise use the config struct read from EEPROM
			  Serial.println("\nEEPROM Read OK");
			  return 0;
		}
		
	} else {					// default creates temporary config pointer
#ifdef RESET_PERSISTENT_CONFIG_ON_DEFAULT
        _temp_config = 0;
        EEPROM.begin(DMXWiFiConfigSIZE);
		_wifi_config = (DMXWiFiconfig*)EEPROM.getDataPtr();
		initConfig();
		commitToPersistentStore();
#else
		_temp_config = 1;	// readFromPersistentStore() will free this pointer and replace it with data from EEPROM
		_wifi_config = (DMXWiFiconfig*) malloc(sizeof(DMXWiFiconfig));
		initConfig();
#endif
		Serial.println("\nDefault configuration.");
	}
	
	return 1;
}

void DMXwifiConfig::initConfig(void) {
  //zero the complete config struct
  memset(_wifi_config, 0, DMXWiFiConfigSIZE);
  
  strncpy((char*)_wifi_config, CONFIG_PACKET_IDENT, 8); //add ident
  _wifi_config->version = DMXWIFI_CONFIG_VERSION;
  _wifi_config->wifi_mode = AP_MODE;                // AP_MODE or STATION_MODE
  _wifi_config->protocol_flags = MULTICAST_MODE;     // sACN multicast mode
  																	 // optional: | INPUT_TO_NETWORK_MODE specify ARTNET_MODE or SACN_MODE
  																	 // optional: | STATIC_MODE   to use static not dhcp address for station
                                        				 // eg. _wifi_config->protocol_flags = MULTICAST_MODE | INPUT_TO_NETWORK_MODE | SACN_MODE;
  strncpy(_wifi_config->ssid, "ESP-DMX-WiFi", 63);
  strncpy(_wifi_config->pwd, "*****", 63);
  _wifi_config->ap_address    = IPAddress(10,110,115,10);       // ip address of access point
  _wifi_config->ap_gateway    = IPAddress(10,110,115,10);
  _wifi_config->ap_subnet     = IPAddress(255,255,255,0);       // match what is passed to dchp connection from computer
  _wifi_config->sta_address   = IPAddress(10,110,115,15);       // station's static address for STATIC_MODE
  _wifi_config->sta_gateway   = IPAddress(192,168,1,1);
  _wifi_config->sta_subnet    = IPAddress(255,0,0,0);
  _wifi_config->multi_address = IPAddress(239,255,0,1);         // sACN multicast address should match universe
  _wifi_config->sacn_universe      = 1;
  _wifi_config->sacn_universe_hi   = 0;
  _wifi_config->artnet_portaddr_lo = 0;
  _wifi_config->artnet_portaddr_hi = 0;
  _wifi_config->device_address  = 1;
  strcpy((char*)_wifi_config->node_name, "com.claudeheintzdesign.esp-dmx");
  _wifi_config->input_address = IPAddress(10,255,255,255);
}

char* DMXwifiConfig::SSID(void) {
	return _wifi_config->ssid;
}

char* DMXwifiConfig::password(void) {
	return _wifi_config->pwd;
}

bool DMXwifiConfig::APMode(void) {
	return ( _wifi_config->wifi_mode == AP_MODE );
}

bool DMXwifiConfig::staticIPAddress(void) {
	return ( _wifi_config->protocol_flags & STATIC_MODE );
}

void DMXwifiConfig::setStaticIPAddress(uint8_t staticip) {
   if ( staticip ) {
      _wifi_config->protocol_flags |= STATIC_MODE;
   } else {
      _wifi_config->protocol_flags &= ~STATIC_MODE;
   }
}

bool DMXwifiConfig::artnetMode(void) {
	return ( ( _wifi_config->protocol_flags & SACN_MODE ) == 0 );
}

bool DMXwifiConfig::sACNMode(void) {
	return ( _wifi_config->protocol_flags & SACN_MODE );
}

bool DMXwifiConfig::multicastMode(void) {
	return ( _wifi_config->protocol_flags & MULTICAST_MODE );
}

bool DMXwifiConfig::inputToNetworkMode(void) {
	return ( _wifi_config->protocol_flags & INPUT_TO_NETWORK_MODE );
}

bool DMXwifiConfig::rdmMode(void) {
  return ( _wifi_config->protocol_flags & RDM_MODE );
}

IPAddress DMXwifiConfig::apIPAddress(void) {
	return _wifi_config->ap_address;
}

IPAddress DMXwifiConfig::apGateway(void) {
	return _wifi_config->ap_gateway;
}

IPAddress DMXwifiConfig::apSubnet(void) {
	return _wifi_config->ap_subnet;
}

IPAddress DMXwifiConfig::stationIPAddress(void) {
	return _wifi_config->sta_address;
}

void DMXwifiConfig::setStationIPAddress ( IPAddress addr ) {
   _wifi_config->sta_address = addr;
}

IPAddress DMXwifiConfig::stationGateway(void) {
	return _wifi_config->sta_gateway;
}

IPAddress DMXwifiConfig::stationSubnet(void) {
	return _wifi_config->sta_subnet;
}

void DMXwifiConfig::setStationSubnetMask ( IPAddress submask ) {
   _wifi_config->sta_subnet = submask;
}

IPAddress DMXwifiConfig::multicastAddress(void) {
	return _wifi_config->multi_address;
}

IPAddress DMXwifiConfig::inputAddress(void) {
	return _wifi_config->input_address;
}

uint16_t DMXwifiConfig::deviceAddress(void) {
	return _wifi_config->device_address;
}

uint16_t DMXwifiConfig::sACNUniverse(void) {
	return _wifi_config->sacn_universe | (_wifi_config->sacn_universe_hi << 8);
}

uint16_t DMXwifiConfig::artnetPortAddress(void) {
	return _wifi_config->artnet_portaddr_lo | ( _wifi_config->artnet_portaddr_hi << 8 );
}

void DMXwifiConfig::setArtNetPortAddress(uint16_t u) {
	_wifi_config->artnet_portaddr_lo = u & 0xff;
	_wifi_config->artnet_portaddr_hi = ( u >> 8 ) & 0xff;
}

char* DMXwifiConfig::nodeName(void) {
	_wifi_config->node_name[31] = 0;	//insure zero termination
	return (char*)_wifi_config->node_name;
}

void DMXwifiConfig::setNodeName(char* nn) {
	strncpy((char*)_wifi_config->node_name, nn, 31);
	_wifi_config->node_name[31] = 0;
}

uint16_t  DMXwifiConfig::numberOfSlots ( void ) {
	return _wifi_config->scene_slots;
}

void DMXwifiConfig::setNumberOfSlots ( uint16_t n ) {
	_wifi_config->scene_slots= n;
}

uint8_t DMXwifiConfig::getSlot ( uint16_t slot ) {
	return _wifi_config->scene[slot-1];
}

void DMXwifiConfig::setSlot ( uint16_t slot, uint8_t level ) {
	_wifi_config->scene[slot-1] = level;
}


void DMXwifiConfig::copyConfig(uint8_t* pkt, uint8_t size) {
	if (( size < DMXWiFiConfigMinSIZE ) || ( size > DMXWiFiConfigSIZE)) {
		return;	//validate incoming size
	}
	uint8_t k;
	uint8_t s = size;
	if ( size < 203 ) {			//does not include nodeName
		s = DMXWiFiConfigMinSIZE;
	}
   memcpy(_wifi_config, pkt, s);
   _wifi_config->opcode = 0;
}

void DMXwifiConfig::readFromPersistentStore(void) {
	if ( _temp_config ) {
		free(_wifi_config);
		_temp_config = 0;
		EEPROM.begin(DMXWiFiConfigSIZE);
		_wifi_config = (DMXWiFiconfig*)EEPROM.getDataPtr();
		
		// zero scene if slots is zero
		if ( numberOfSlots() == 0 ) {
			for(int i=1; i<513; i++) {
				setSlot(i,0);
			}
		}
    
	}
}

void DMXwifiConfig::commitToPersistentStore(void) {
	_wifi_config->opcode = 0;
	EEPROM.write(8,1);  // set byte to 1 so that next write causes dirty flag to set
	EEPROM.write(8,0);  //zero term. for ident sets dirty flag enabling commit()
	if ( EEPROM.commit() ) {
		Serial.println("EEPROM commit OK");
	} else {
		Serial.println("EEPROM commit failed");
	}
}

uint8_t* DMXwifiConfig::config(void) {
	return (uint8_t*) _wifi_config;
}

uint8_t DMXwifiConfig::configSize(void) {
	return DMXWiFiConfigSIZE;
}

void DMXwifiConfig::hidePassword(void) {
  strncpy(_save_pwd, _wifi_config->pwd, 63);
  memset(_wifi_config->pwd, 0, 64);
  strncpy(_wifi_config->pwd, "********", 8);
}

void DMXwifiConfig::restorePassword(void) {
  memset(_wifi_config->pwd, 0, 64);
  strncpy(_wifi_config->pwd, _save_pwd, 63);
}
