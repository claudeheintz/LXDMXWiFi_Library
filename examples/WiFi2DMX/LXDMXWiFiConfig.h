/**************************************************************************/
/*!
    @file     LXDMXWiFiconfig.h
    @author   Claude Heintz
    @license  BSD (see LXDMXWiFi.h or http://lx.claudeheintzdesign.com/opensource.html)
    @copyright 2016 by Claude Heintz All Rights Reserved

    Edit initConfig for your own default settings.
*/
/**************************************************************************/

#include <inttypes.h>
#include <string.h>

typedef struct dmxwifiConfig {
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
} DMXWiFiConfig;

#define CONFIG_PACKET_IDENT "ESP-DMX"
#define DMXWiFiConfigSIZE 232

#define STATION_MODE 0
#define AP_MODE 1

#define ARTNET_MODE 0
#define SACN_MODE 1
#define STATIC_MODE 2
#define MULTICAST_MODE 4

#define OUTPUT_FROM_NETWORK_MODE 0
#define INPUT_TO_NETWORK_MODE 8

/*
   initConfig initializes the DMXWiFiConfig structure with default settings
   The default is to receive Art-Net with the WiFi configured as an access point.
   (Modify the initConfig to change default settings.  But, highly recommend leaving AP_MODE for default startup.)
*/
void initConfig(DMXWiFiConfig* cfptr);


/* 
   Utility function. WiFi station password should never be returned by query.
*/
void erasePassword(DMXWiFiConfig* cfptr);

