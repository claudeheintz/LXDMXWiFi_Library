/* LXDMXWiFiConfig.h
   Copyright 2015-2016 by Claude Heintz Design
   All rights reserved.
-----------------------------------------------------------------------------------
*/

struct DMXWiFiConfig {
   char    ident[8];      // ESP-DMX\0
   uint8_t opcode;		  // data = 0, query = '?', set ='!'
   char    ssid[64];      // max is actually 32
   char    pwd[64];       // depends on security 8, 13, 8-63
   uint8_t wifi_mode;
   uint8_t protocol_mode;
   uint8_t ap_chan;			//unimplemented
   uint32_t ap_address;
   uint32_t ap_gateway;		//140
   uint32_t ap_subnet;
   uint32_t sta_address;
   uint32_t sta_gateway;
   uint32_t sta_subnet;
   uint32_t multi_address;
   uint8_t sacn_universe;   //should match multicast address
   uint8_t artnet_subnet;
   uint8_t artnet_universe;
   uint8_t node_name[32];
   uint32_t input_address;
   uint8_t reserved[25];
};

#define ESPDMX_IDENT "ESP-DMX"
#define DMXWiFiConfigSIZE 232

#define STATION_MODE 0
#define AP_MODE 1

#define ARTNET_MODE 0
#define SACN_MODE 1
#define STATIC_MODE 2
#define MULTICAST_MODE 4

#define OUTPUT_FROM_NETWORK_MODE 0
#define INPUT_TO_NETWORK_MODE 8
