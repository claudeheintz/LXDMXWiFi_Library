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
#include "LXDMXWiFiConfig.h"

/**
 * This is the single object used for accessing the configuration settings
 */
DMXwifiConfig DMXWiFiConfig;

/**
 * This is static storage for the configuration settings.  The compiler allocates it in flash memory.
 * It occupies a complete row of flash, comprised of 4 @ 64 byte pages
 */
__attribute__((__aligned__(256))) static const uint8_t _config_in_flash[256] {};    //used to use DMXWiFiConfigSIZE

/*******************************************************************************
 ***********************  DMXwifiConfig member functions  ********************/

DMXwifiConfig::DMXwifiConfig ( void ) {
	_wifi_config = 0;
}

DMXwifiConfig::~DMXwifiConfig ( void ) {
	if ( _wifi_config != 0 ) {
		delete _wifi_config;
	}
}

uint8_t DMXwifiConfig::begin ( uint8_t mode ) {
	_wifi_config = new ( DMXWiFiconfig );
	if ( mode ) {
		return readFromPersistentStore();
	}
	initConfig();
	return 1;
}

/**
 * initConfig initializes the fields of the _wifi_config struct.
 */

void DMXwifiConfig::initConfig(void) {
  //zero the complete config struct
  memset(_wifi_config, 0, DMXWiFiConfigSIZE);
  
  strncpy((char*)_wifi_config, CONFIG_PACKET_IDENT, 8); //add ident
  _wifi_config->version = 1;
  strncpy(_wifi_config->ssid, "MKR-DMX-WiFi", 63);
  strncpy(_wifi_config->pwd, "****", 63);
  _wifi_config->wifi_mode = AP_MODE;                // AP_MODE or STATION_MODE
  _wifi_config->protocol_flags = MULTICAST_MODE | RDM_MODE;    // sACN multicast mode
  																	 // optional: | INPUT_TO_NETWORK_MODE specify ARTNET_MODE or SACN_MODE
  																	 // optional: | STATIC_MODE   to use static not dhcp address for station
                                     // optional: | RDM_MODE      enables RDM (requires v2.0 SAMD21DMX driver)
                                        				 // eg. _wifi_config->protocol_flags = MULTICAST_MODE | INPUT_TO_NETWORK_MODE | SACN_MODE;
  _wifi_config->ap_address    = IPAddress(192,168,1,1);       // ip address of access point
  _wifi_config->ap_gateway    = IPAddress(192,168,1,1);
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
  strcpy((char*)_wifi_config->node_name, "com.claudeheintzdesign.d21-dmx");
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

bool DMXwifiConfig::rdmMode(void) {
  return ( _wifi_config->protocol_flags & RDM_MODE );
}

bool DMXwifiConfig::inputToNetworkMode(void) {
	return ( _wifi_config->protocol_flags & INPUT_TO_NETWORK_MODE );
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

void DMXwifiConfig::copyConfig(uint8_t* pkt, uint8_t size) {
	if (( size < 171 ) || ( size > DMXWiFiConfigSIZE)) {
		return;	//validate incoming size
	}
	uint8_t k;
	uint8_t s = size;
	if ( size < 203 ) {
		s = 171;
	}
   memcpy(_wifi_config, pkt, s);
   _wifi_config->opcode = 0;
}

/**
 * copies from the static byte array into the _wifi_config structure.
 * If the static storage is not valid, readFromPersistentStore calls initConfig
 * and writes the default settings to flash
 */

uint8_t DMXwifiConfig::readFromPersistentStore(void) {
  	copyConfig((uint8_t*)_config_in_flash, DMXWiFiConfigSIZE);
  	uint8_t did_default = 1;

  	// check to see if ident matches
  	if ( strcmp(CONFIG_PACKET_IDENT, _wifi_config->ident) != 0 ) {
  		initConfig();										// invalid ident, init & store default
  		//commitToPersistentStore();      // store initialized default config... may not be needed, no different result if stored or by calling initConfig
  	} else {
  		_wifi_config->opcode = 0;
  		did_default = 0;
  	}
  	return did_default;
}

uint32_t packBytes(uint8_t* p) {
  return p[0] | ( p[1] << 8 ) | (p[2] << 16) | (p[3] << 24);
}

/**
 * commitToPersistentStore writes to the statically allocated row of flash memory.
 * The row needs to be erased first. (data sheet 384, procedure pg 388)
 * Each of the 4 pages that make up the row are then written to flash
 * This follows the procedure for manual write on page 388 of the datasheet.
 */

void DMXwifiConfig::commitToPersistentStore(void) {
  /* Clear previous error flags (?)  May want to check for locked...*/
    NVMCTRL->STATUS.reg |= NVMCTRL_STATUS_MASK;
    
		while ( NVMCTRL->INTFLAG.bit.READY == 0 ) {
		  delay(5);
		} 	//wait until nvm controller is ready
    
    // first erase the row (all 256 bytes)
    const volatile void* flash_address = _config_in_flash;
    
		NVMCTRL->ADDR.reg =  ((uint32_t)flash_address) / 2;                   //16bit address see pg 403

		NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_ER;  // execution stopping here 
    
		while ( NVMCTRL->INTFLAG.bit.READY == 0 ) {
		  delay(5);
		} 	//wait until nvm controller is ready pg 400
		
		uint8_t* p = (uint8_t*) _wifi_config;
      volatile uint32_t* fp = (volatile uint32_t *)_config_in_flash;
		uint8_t pk = 0;
		
		NVMCTRL->CTRLB.bit.MANW = 0x1; //set manual write mode
    
		// write out the pages in the row, one at a time
		for (int k=0; k<4; k++) {
			//clear the page buffer 
        NVMCTRL->ADDR.reg = ((uint32_t)fp)/2; //set ADDR register before pg buffer clear? probably unneeded see pg 388 | 403

			  NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_PBC;
			  while ( NVMCTRL->INTFLAG.bit.READY == 0 ) {} 	//wait until nvm controller is ready

     		/* Clear error flags (from Atmel EEPROM simulator sdk code)*/
      	NVMCTRL->STATUS.reg |= NVMCTRL_STATUS_MASK;
		
			//copy directly into address space up to 16 uint32_t addresses (16*4 = 64bytes/page)
			for (uint8_t ni=0;((ni<16) && (pk<DMXWiFiConfigSIZE)); ni++) {
        		*fp = packBytes(p);
				fp++;
				p+=4;
				pk+=4;
			}
			
			//execute the write  (see pg 403, ADDR register is set when *fp is assigned)
			NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_WP;
			while ( NVMCTRL->INTFLAG.bit.READY == 0 ) {} 	//wait until nvm controller is ready
		}	//page writes

}   //commitToPersistentStore

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

