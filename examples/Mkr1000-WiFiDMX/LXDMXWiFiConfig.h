/**************************************************************************/
/*!
    @file     LXDMXWiFiconfig.h
    @author   Claude Heintz
    @license  BSD (see LXDMXWiFi.h or http://lx.claudeheintzdesign.com/opensource.html)
    @copyright 2016 by Claude Heintz All Rights Reserved
    
    
*/
/**************************************************************************/

#include <inttypes.h>
#include <string.h>

/* 
	structure for storing WiFi and Protocol configuration settings
*/

typedef struct dmxwifiConfig {
   char    ident[8];      // ESP-DMX\0                                              0
   uint8_t opcode;		  // data = 0, query = '?', set ='!'                          8
   uint8_t version;		  // currently 1                                              9
   uint8_t wifi_mode;                                                          //   10
   uint8_t protocol_flags;                                                     //   11
   char    ssid[64];      // max is actually 32                                     12
   char    pwd[64];       // depends on security 8, 13, 8-63                        76
   uint32_t ap_address;		// static IP address of access point                      140
   uint32_t ap_gateway;		//   gateway in access point mode                         144
   uint32_t ap_subnet;		//   subnet  in access point mode                         148
   uint32_t sta_address;	// static IP address in station mode (! DHCP bit set)     152
   uint32_t sta_gateway;	//   gateway in station mode                              156
   uint32_t sta_subnet;		//   subnet  in station mode                              160
   uint32_t multi_address;  // multicast address for sACN                           164
   uint8_t sacn_universe;   // should match multicast address                       168
   uint8_t artnet_portaddr_hi;                                                  //  169
   uint8_t artnet_portaddr_lo;                                                  //  170
   uint8_t sacn_universe_hi;		 // backwards compatability                         171
   uint8_t node_name[32];                                                       //  172
   uint32_t input_address;  // IP address for sending DMX to network in input mode  204
   uint16_t device_address; // dmx address (if applicable)                          208
   uint8_t reserved[22];                                                        //  210
} DMXWiFiconfig;                                                                //  232

#define CONFIG_PACKET_IDENT "ESP-DMX"
#define DMXWiFiConfigSIZE 232

#define STATION_MODE 0
#define AP_MODE 1

#define ARTNET_MODE           0
#define SACN_MODE             1
#define STATIC_MODE           2
#define MULTICAST_MODE        4
#define INPUT_TO_NETWORK_MODE 8
#define RDM_MODE              16 

#define OUTPUT_FROM_NETWORK_MODE 0


/*!   
@class DMXwifiConfig
@abstract
   DMXwifiConfig abstracts WiFi and Protocol configuration settings so that they can
   be saved and retrieved from persistent storage.
*/

class DMXwifiConfig {

  public:
  
	  DMXwifiConfig ( void );
	 ~DMXwifiConfig( void );
	 
	 /* 
	 	handles init of config data structure, reading from persistent if desired.
	 	returns 1 if boot uses default settings, 0 if settings are read from EEPROM
	 */
	 uint8_t begin ( uint8_t mode );
	 
	 /*
	 initConfig initializes the DMXWiFiConfig structure with default settings
	 The default is to receive Art-Net with the WiFi configured as an access point.
	 (Modify the initConfig to change default settings.  But, highly recommend leaving AP_MODE for default startup.)
	 */
	 void initConfig(void);
	 
	 /* 
	 	WiFi setup parameters
	 */
	 char* SSID(void);
	 char* password(void);
	 bool APMode(void);
	 bool staticIPAddress(void);
	 void setStaticIPAddress(uint8_t staticip);
	 
	 /* 
	 	protocol modes
	 */
    bool artnetMode(void);
    bool sACNMode(void);
    bool multicastMode(void);
    bool rdmMode(void);
    bool inputToNetworkMode(void);
    
    /* 
	 	stored IPAddresses
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
	 uint8_t readFromPersistentStore(void);
	 
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
	 	space to save password
	 */
     char    _save_pwd[64];
  	 
};

extern DMXwifiConfig DMXWiFiConfig;
