#include <IPAddress.h>
#include "LXDMXWiFiConfig.h"

void initConfig(DMXWiFiConfig* cfptr) {
  //zero the complete config struct
  uint8_t* p = (uint8_t*) cfptr;
  uint8_t k;
  for(k=0; k<DMXWiFiConfigSIZE; k++) {
    p[k] = 0;
  }
  
  strncpy((char*)cfptr, CONFIG_PACKET_IDENT, 8); //add ident
  strncpy(cfptr->ssid, "ESP-DMX-WiFi", 63);
  strncpy(cfptr->pwd, "*****", 63);
  cfptr->wifi_mode = AP_MODE;                       // AP_MODE or STATION_MODE
  cfptr->protocol_mode = SACN_MODE;     // ARTNET_MODE or SACN_MODE ( plus optional: | STATIC_MODE, | MULTICAST_MODE, | INPUT_TO_NETWORK_MODE )
                                        // eg. cfptr->protocol_mode = SACN_MODE | MULTICAST_MODE ;
  cfptr->ap_chan = 2;
  cfptr->ap_address    = IPAddress(192,168,1,1);       // ip address of access point
  cfptr->ap_gateway    = IPAddress(192,168,1,1);
  cfptr->ap_subnet     = IPAddress(255,255,255,0);       // match what is passed to dchp connection from computer
  cfptr->sta_address   = IPAddress(10,110,115,15);       // station's static address for STATIC_MODE
  cfptr->sta_gateway   = IPAddress(192,168,1,1);
  cfptr->sta_subnet    = IPAddress(255,0,0,0);
  cfptr->multi_address = IPAddress(239,255,0,1);         // sACN multicast address should match universe
  cfptr->sacn_universe   = 1;
  cfptr->artnet_universe = 0;
  cfptr->artnet_subnet   = 0;
  strcpy((char*)cfptr->node_name, "com.claudeheintzdesign.d21-dmx");
  cfptr->input_address = IPAddress(10,255,255,255);
}

void erasePassword(DMXWiFiConfig* cfptr) {
  strncpy(cfptr->pwd, "********", 63);
}
