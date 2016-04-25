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

}

void DMXwifiConfig::begin ( uint8_t mode ) {
	EEPROM.begin(DMXWiFiConfigSIZE);                      		// get pointer to data
  	_wifi_config = (DMXWiFiconfig*)EEPROM.getDataPtr();
	
	if ( mode ) {
		// data already read, check initialization
		if ( strcmp(CONFIG_PACKET_IDENT, (const char *) _wifi_config) != 0 ) {	// if structure not previously stored
		  initConfig();																		// initialize and store in EEPROM
		  commitToPersistentStore();
		  Serial.println("\nInitialized EEPROM");
		} else {																					// otherwise use the config struct read from EEPROM
			  Serial.println("\nEEPROM Read OK");
		}
	} else {
		initConfig();
		Serial.println("\nDefault configuration.");
	}
}

void DMXwifiConfig::initConfig(void) {
  //zero the complete config struct
  memset(_wifi_config, 0, DMXWiFiConfigSIZE);
  
  strncpy((char*)_wifi_config, CONFIG_PACKET_IDENT, 8); //add ident
  strncpy(_wifi_config->ssid, "ESP-DMX-WiFi", 63);
  strncpy(_wifi_config->pwd, "*****", 63);
  _wifi_config->wifi_mode = AP_MODE;                       // AP_MODE or STATION_MODE
  _wifi_config->protocol_mode = ARTNET_MODE;     // ARTNET_MODE or SACN_MODE ( plus optional: | STATIC_MODE, | MULTICAST_MODE, | INPUT_TO_NETWORK_MODE )
                                        // eg. _wifi_config->protocol_mode = SACN_MODE | MULTICAST_MODE ;
  _wifi_config->ap_chan = 2;
  _wifi_config->ap_address    = IPAddress(10,110,115,10);       // ip address of access point
  _wifi_config->ap_gateway    = IPAddress(10,110,115,10);
  _wifi_config->ap_subnet     = IPAddress(255,255,255,0);       // match what is passed to dchp connection from computer
  _wifi_config->sta_address   = IPAddress(10,110,115,15);       // station's static address for STATIC_MODE
  _wifi_config->sta_gateway   = IPAddress(192,168,1,1);
  _wifi_config->sta_subnet    = IPAddress(255,0,0,0);
  _wifi_config->multi_address = IPAddress(239,255,0,1);         // sACN multicast address should match universe
  _wifi_config->sacn_universe   = 1;
  _wifi_config->artnet_universe = 0;
  _wifi_config->artnet_subnet   = 0;
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
	return ( _wifi_config->protocol_mode & STATIC_MODE );
}

bool DMXwifiConfig::artnetMode(void) {
	return ( ( _wifi_config->protocol_mode & SACN_MODE ) == 0 );
}

bool DMXwifiConfig::sACNMode(void) {
	return ( _wifi_config->protocol_mode & SACN_MODE );
}

bool DMXwifiConfig::multicastMode(void) {
	return ( _wifi_config->protocol_mode & MULTICAST_MODE );
}

bool DMXwifiConfig::inputToNetworkMode(void) {
	return ( _wifi_config->protocol_mode & INPUT_TO_NETWORK_MODE );
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

IPAddress DMXwifiConfig::stationGateway(void) {
	return _wifi_config->sta_gateway;
}

IPAddress DMXwifiConfig::stationSubnet(void) {
	return _wifi_config->sta_subnet;
}

IPAddress DMXwifiConfig::multicastAddress(void) {
	return _wifi_config->multi_address;
}

IPAddress DMXwifiConfig::inputAddress(void) {
	return _wifi_config->input_address;
}

uint8_t DMXwifiConfig::sACNUniverse(void) {
	return _wifi_config->sacn_universe;
}

uint8_t DMXwifiConfig::artnetSubnet(void) {
	return _wifi_config->artnet_subnet;
}

uint8_t DMXwifiConfig::artnetUniverse(void) {
	return _wifi_config->artnet_universe;
}

void DMXwifiConfig::setArtNetUniverse(int u) {
	_wifi_config->artnet_subnet = (u & 0x0F);
	_wifi_config->artnet_universe = ((u>>4) & 0x0F);
}

char* DMXwifiConfig::nodeName(void) {
	_wifi_config->node_name[31] = 0;	//insure zero termination
	return (char*)_wifi_config->node_name;
}

void DMXwifiConfig::setNodeName(char* nn) {
	strncpy((char*)_wifi_config->node_name, nn, 31);
	_wifi_config->node_name[31] = 0;
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
	//data is cached must end EEPROM and call begin again
}

void DMXwifiConfig::commitToPersistentStore(void) {
	_wifi_config->opcode = 0;
	EEPROM.write(8,0);  //zero term. for ident sets dirty flag 
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
