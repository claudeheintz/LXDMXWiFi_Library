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



/*
   initConfig initializes the DMXWiFiConfig structure with default settings
   The default is to receive Art-Net with the WiFi configured as an access point.
   (Modify the initConfig to change default settings.  But, highly recommend leaving AP_MODE for default startup.)
*/
void initConfig(DMXWiFiConfig* cfptr) {
  //zero the complete config struct
  uint8_t* p = (uint8_t*) cfptr;
  uint8_t k;
  for(k=0; k<DMXWiFiConfigSIZE; k++) {
    p[k] = 0;
  }
  
  strncpy((char*)cfptr, ESPDMX_IDENT, 8); //add ident
  strncpy(cfptr->ssid, "ESP-DMX-WiFi", 63);
  strncpy(cfptr->pwd, "********", 63);
  cfptr->wifi_mode = AP_MODE;                       // AP_MODE or STATION_MODE
  cfptr->protocol_mode = ARTNET_MODE;     // ARTNET_MODE or SACN_MODE
                                                         // optional | STATIC_MODE or | MULTICAST_MODE or | INPUT_TO_NETWORK_MODE
  cfptr->ap_chan = 2;
  cfptr->ap_address    = IPAddress(10,110,115,10);       // ip address of access point
  cfptr->ap_gateway    = IPAddress(10,110,115,1);
  cfptr->ap_subnet     = IPAddress(255,255,255,0);       // match what is passed to dchp connection from computer
  cfptr->sta_address   = IPAddress(10,110,115,15);       // station's static address for STATIC_MODE
  cfptr->sta_gateway   = IPAddress(192,168,1,1);
  cfptr->sta_subnet    = IPAddress(255,0,0,0);
  cfptr->multi_address = IPAddress(239,255,0,1);         // sACN multicast address should match universe
  cfptr->sacn_universe   = 1;
  cfptr->artnet_universe = 0;
  cfptr->artnet_subnet   = 0;
  strcpy((char*)cfptr->node_name, "com.claudeheintzdesign.esp-dmx");
  cfptr->input_address = IPAddress(10,255,255,255);
}


/* 
   Utility function. WiFi station password should never be returned by query.
*/
void erasePassword(DMXWiFiConfig* cfptr) {
  strncpy(cfptr->pwd, "********", 63);
}
