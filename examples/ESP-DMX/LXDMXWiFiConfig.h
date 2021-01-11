/**************************************************************************/
/*!
    @file     LXDMXWiFiconfig.h
    @author   Claude Heintz
    @license  BSD (see LXDMXWiFi.h or http://lx.claudeheintzdesign.com/opensource.html)
    @copyright 2016-2018 by Claude Heintz All Rights Reserved
    
    
*/
/**************************************************************************/

#include <inttypes.h>
#include <string.h>
#include "LXDMXWiFi.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

/* 
	structure for storing WiFi and Protocol configuration settings
*/

typedef struct dmxwifiConfig {
   char    ident[8];      // ESP-DMX\0
   uint8_t opcode;		  // data = 0, query = '?', set ='!'
   uint8_t version;		  // currently 1
   uint8_t wifi_mode;
   uint8_t protocol_flags;
   char    ssid[64];      // max is actually 32
   char    pwd[64];       // depends on security 8, 13, 8-63
   uint32_t ap_address;		// static IP address of access point
   uint32_t ap_gateway;		//   gateway in access point mode
   uint32_t ap_subnet;		//   subnet  in access point mode
   uint32_t sta_address;	// static IP address in station mode (! DHCP bit set)
   uint32_t sta_gateway;	//   gateway in station mode
   uint32_t sta_subnet;		//   subnet  in station mode
   uint32_t multi_address;  // multicast address for sACN
   uint8_t sacn_universe;   // should match multicast address
   uint8_t artnet_portaddr_hi;
   uint8_t artnet_portaddr_lo;
   uint8_t sacn_universe_hi;		 // backwards compatability
   uint8_t node_name[32];
   uint32_t input_address;  // IP address for sending DMX to network in input mode
   uint16_t device_address; // dmx address (if applicable)
   uint8_t reserved[22];
} DMXWiFiconfig;

#define CONFIG_PACKET_IDENT "ESP-DMX"
#define DMXWiFiConfigSIZE 232
#define DMXWiFiConfigMinSIZE 171
#define DMXWIFI_CONFIG_VERSION 1
#define DMXWIFI_CONFIG_INVALID_VERSION 27

#define LX_STATION_MODE  0
#define LX_AP_MODE       1

#define ARTNET_MODE           0
#define SACN_MODE             1
#define STATIC_MODE           2
#define MULTICAST_MODE        4
#define INPUT_TO_NETWORK_MODE 8
#define RDM_MODE              16 

#define CONFIG_NO_MESSAGES    0
#define CONFIG_PRINT_MESSAGES 1

typedef void (*IndicateActivityCallback)(void);

/*!   
@class DMXwifiConfig
@abstract
   DMXwifiConfig abstracts WiFi and Protocol configuration settings so that they can
   be saved and retrieved from persistent storage.
   
@discussion
    To allow use of the configuration utility use DMXwifiConfig.begin(1);
    When using remote configuration:
       The remote configuration utility can be used to edit the settings without re-loading the sketch.
       Settings from persistent memory.
       Calling DMXwifiConfig.begin(0) temporarily uses the settings in the LXDMXWiFiConfig.initConfig() method.
       This insures there is a default way of connecting to the sketch in order to use the remote utility,
       even if it is configured to use a WiFi network that is unavailable.
 
    Without remote configuration, ( using DMXwifiConfig.begin() or DMXwifiConfig.begin(0) ),
       settings are read from the LXDMXWiFiConfig.initConfig() method.
       In this case, it is necessary to edit that function and recompile the sketch in order to change the settings.
*/

class DMXwifiConfig {

  public:
  
	  DMXwifiConfig ( void );
	 ~DMXwifiConfig( void );
	 
	 /* 
	 	handles init of config data structure, reading from persistent if desired.
	 	returns 1 if boot uses default settings, 0 if settings are read from EEPROM
	 */
	 uint8_t begin ( uint8_t mode = 0 );
	 
	 /*
	 initConfig initializes the DMXWiFiConfig structure with default settings
	 The default is to receive Art-Net with the WiFi configured as an access point.
	 (Modify the initConfig to change default settings.  But, highly recommend leaving AP_MODE for default startup.)
	 */
	 void initConfig(void);
  
    /*
    initialize the WiFi connection based on config settings
   */
   uint8_t setupWiFi(IndicateActivityCallback indicateConnecting);

    /*
     Check to see if packet is a config packet.
  
     In the case it is a query, it replies with the current config from persistent storage.
     
     In the case of upload, it copies the payload to persistent storage
     and also replies with the config settings.
    */
   uint8_t checkConfigReceived(LXDMXWiFi* interface, WiFiUDP cUDP, IndicateActivityCallback informUser, uint8_t printMessages);
   
  	 
	 /* 
	 	WiFi setup parameter accessors
	 */
	 char* SSID(void);
	 char* password(void);
	 bool APMode(void);
	 bool staticIPAddress(void);
	 void setStaticIPAddress(uint8_t staticip);
	 
	 /* 
	 	protocol mode accressors
	 */
    bool artnetMode(void);
    bool sACNMode(void);
    bool multicastMode(void);
    bool rdmMode(void);
    bool inputToNetworkMode(void);
    
    /* 
	 	stored IPAddress accessors
	 */
    IPAddress apIPAddress(void);
    IPAddress apGateway(void);
	 IPAddress apSubnet(void);
	 IPAddress stationIPAddress(void);
	 void      setStationIPAddress ( IPAddress addr );
    IPAddress stationGateway(void);
	 IPAddress stationSubnet(void);
	 void      setStationSubnetMask ( IPAddress submask );
	 IPAddress multicastAddress(void);
	 IPAddress inputAddress(void);
	 
	 /*
	   dmx address
	 */
	 uint16_t deviceAddress(void);
	 
	 /* 
	 	protocol settings
	 */
	 uint16_t sACNUniverse(void);
	 uint16_t artnetPortAddress(void);
	 void setArtNetPortAddress(uint16_t u);
	 char* nodeName(void);
	 void setNodeName(char* nn);
	 
	 /* 
	 	copyConfig from uint8_t array
	 */
	 void copyConfig(uint8_t* pkt, uint8_t size);
	 
	 /* 
	 	read from EEPROM or flash
	 */
	 void readFromPersistentStore(void);
	 
	 /* 
	 	write to EEPROM or flash
	 */
	 void commitToPersistentStore(void);
	 
	 /* 
	 	pointer and size for UDP.write()
	 */
	 uint8_t* config(void);
	 uint8_t configSize(void);

	 /* 
	 	Utility function. WiFi station password should never be returned by query.
	 */
	 void hidePassword(void);
	 void restorePassword(void);

  private:
   
    /* 
	 	pointer to configuration structure
	 */
     DMXWiFiconfig* _wifi_config;
    /*
   	saved password (never sent out in reply)
    */
	  char    _save_pwd[64];
	 /*
		flag indicating _wifi_config points to allocated memory not EEPROM address
	 */
	  uint8_t _temp_config;
};

extern DMXwifiConfig DMXWiFiConfig;
