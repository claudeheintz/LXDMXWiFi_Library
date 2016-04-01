/**************************************************************************/
/*!
    @file     WiFi2DMX.ino
    @author   Claude Heintz
    @license  BSD (see LXDMXWiFi.h)
    @copyright 2015 by Claude Heintz All Rights Reserved

    Example using LXDMXWiFi_Library for output of Art-Net or E1.31 sACN from
    ESP 8266 WiFi connection to DMX.  This version 
    
    Art-Net(TM) Designed by and Copyright Artistic Licence (UK) Ltd
    sACN E 1.31 is a public standard published by the PLASA technical standards program
    
    Note:  For sending packets larger than 512 bytes, ESP8266 WiFi Library v2.1
           is necessary.
           
           This example requires the LXESP8266DMX library for DMX serial output
           https://github.com/claudeheintz/LXESP8266DMX

    @section  HISTORY

    v1.0 - First release
    v2.0 - Added remote configuration with DMXWiFiConfig packets
           Configure by holding pin 16 low on power up to force access point mode.
           Use python script esp_dmx_wifi_config.py to examine & change protocol and connection settings
*/
/**************************************************************************/

#include <LXESP8266UARTDMX.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <LXDMXWiFi.h>
#include <LXWiFiArtNet.h>
#include <LXWiFiSACN.h>
#include <EEPROM.h>

#define STARTUP_MODE_PIN 16      // pin for force default setup when low (use 10k pullup to insure high)
#define DIRECTION_PIN 4          // pin for output direction enable on MAX481 chip

struct DMXWiFiConfig* esp_config;

// *** modify the following for setting up the default WiFi connection ***

void initConfig(DMXWiFiConfig* cfptr) {
  strncpy((char*)cfptr, ESPDMX_IDENT, sizeof(*esp_config)); //add ident and zero the rest
  strncpy(cfptr->ssid, "ESP-DMX-WiFi", 63);
  strncpy(cfptr->pwd, "********", 63);
  cfptr->wifi_mode = AP_MODE;                       // AP_MODE or STATION_MODE
  cfptr->protocol_mode = ARTNET_MODE;     // ARTNET_MODE or SACN_MODE
                                                         // optional | STATIC_MODE or | MULTICAST_MODE
  cfptr->ap_chan = 2;
  cfptr->ap_address    = IPAddress(10,110,115,10);       // ip address of access point
  cfptr->ap_gateway    = IPAddress(10,1,1,1);
  cfptr->ap_subnet     = IPAddress(255,255,255,0);       // match what is passed to dchp connection from computer
  cfptr->sta_address   = IPAddress(10,110,115,15);       // station's static address for STATIC_MODE
  cfptr->sta_gateway   = IPAddress(192,168,1,1);
  cfptr->sta_subnet    = IPAddress(255,0,0,0);
  cfptr->multi_address = IPAddress(239,255,0,1);         // sACN multicast address should match universe
  cfptr->sacn_universe   = 1;
  cfptr->artnet_universe = 0;
  cfptr->artnet_subnet   = 0;
}

//remove password
void erasePassword(DMXWiFiConfig* cfptr) {
  strncpy(cfptr->pwd, "********", 63);
}

// dmx protocol interface for parsing packets (created in setup)
LXDMXWiFi* interface;

// LX8266DMX instance for DMX output using UART1/GPIO2
LX8266DMX* dmx_driver = new LX8266DMX();

// An EthernetUDP instance to let us send and receive UDP packets
WiFiUDP wUDP;

uint8_t led_state = 0;

void blinkLED() {
  if ( led_state ) {
    digitalWrite(BUILTIN_LED, HIGH);
    led_state = 0;
  } else {
    digitalWrite(BUILTIN_LED, LOW);
    led_state = 1;
  }
}

/************************************************************************

  Setup creates the WiFi connection.
  
  It also creates the network protocol object,
  either an instance of LXWiFiArtNet or LXWiFiSACN.
  
  It then starts listening on the appropriate UDP port.
  
  And, it starts the dmx_driver object sending serial DMX via the UART1 TX pin.
  (see the LXESP8266DMX library documentation for driver details)

*************************************************************************/

void setup() {
  Serial.begin(112500);
  Serial.setDebugOutput(1); //use uart0 for debugging
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(STARTUP_MODE_PIN, INPUT);
  pinMode(DIRECTION_PIN, OUTPUT);
  
  dmx_driver->startOutput();                            // DMX Driver startup
  digitalWrite(DIRECTION_PIN,HIGH);
  
  EEPROM.begin(DMXWiFiConfigSIZE);                      // Config initialization
  esp_config = (DMXWiFiConfig*)EEPROM.getDataPtr();
  if ( digitalRead(STARTUP_MODE_PIN) == 0 ) {
    initConfig(esp_config);
    Serial.println("\ndefault startup");
  } else if ( strcmp(ESPDMX_IDENT, (const char *) esp_config) != 0 ) {
    initConfig(esp_config);
    EEPROM.write(8,0);  //zero term. for ident sets dirty flag 
    EEPROM.commit();
    Serial.println("\nInitialized EEPROM");
  } else {
    Serial.println("\nEEPROM Read OK");
  }

  if ( esp_config->wifi_mode == AP_MODE ) {            // WiFi startup
    WiFi.mode(WIFI_AP);
    WiFi.softAP(esp_config->ssid);
    WiFi.softAPConfig(esp_config->ap_address, esp_config->ap_gateway, esp_config->ap_subnet);
    Serial.print("created access point ");
    Serial.print(esp_config->ssid);
    Serial.print(", ");
  } else {
    Serial.print("wifi connecting... ");
    WiFi.mode(WIFI_STA);
    WiFi.begin(esp_config->ssid,esp_config->pwd);

    // static IP for Art-Net  may need to be edited for a particular network
    if ( esp_config->protocol_mode & STATIC_MODE ) {  
      WiFi.config(esp_config->sta_address, esp_config->sta_gateway, esp_config->sta_subnet);
    }
    
    while (WiFi.status() != WL_CONNECTED) {
      delay(100);
      blinkLED();
    }
  }
  Serial.println("wifi started.");
  
  if ( esp_config->protocol_mode & SACN_MODE ) {         // Initialize network<->DMX interface
    interface = new LXWiFiSACN();
    interface->setUniverse(esp_config->sacn_universe);
  } else {
    interface = new LXWiFiArtNet(WiFi.localIP(), WiFi.subnetMask());
    ((LXWiFiArtNet*)interface)->setSubnetUniverse(esp_config->artnet_subnet, esp_config->artnet_universe);
  }
  Serial.print("interface created, ");
  
  if ( ( esp_config->protocol_mode & MULTICAST_MODE ) ) { // Start listening for UDP on port
    if ( esp_config->wifi_mode == AP_MODE ) {
      wUDP.beginMulticast(WiFi.softAPIP(), esp_config->multi_address, interface->dmxPort());
    } else {
      wUDP.beginMulticast(WiFi.localIP(), esp_config->multi_address, interface->dmxPort());
    }
  } else {
    wUDP.begin(interface->dmxPort());
  }
  Serial.print("udp started,");

  if ( ( esp_config->protocol_mode & SACN_MODE ) == 0 ) { //if needed, announce presence via Art-Net Poll Reply
     ((LXWiFiArtNet*)interface)->send_art_poll_reply(wUDP);
  }

  Serial.println(" setup complete.");
  blinkLED();
}

/************************************************************************

  The main loop checks for and reads packets from WiFi UDP socket
  connection.  readDMXPacket() returns true when a DMX packet is received.
  In which case, the data is copied to the dmx_driver object which is driving
  the UART serial DMX output.

*************************************************************************/

void loop() {
  uint8_t good_dmx = interface->readDMXPacket(wUDP);

  if ( good_dmx ) {
     for (int i = 1; i <= interface->numberOfSlots(); i++) {
        dmx_driver->setSlot(i , interface->getSlot(i));
     }
     blinkLED();
  } else {
    if ( strcmp(ESPDMX_IDENT, (const char *) interface->packetBuffer()) == 0 ) {
      Serial.print("ESP-DMX received, ");
      uint8_t reply = 0;
      if ( interface->packetBuffer()[8] == '?' ) {
        EEPROM.begin(DMXWiFiConfigSIZE);            //deletes data (ie esp_config) and reads from flash
        esp_config = (DMXWiFiConfig*)EEPROM.getDataPtr();
        reply = 1;
      } else if (( interface->packetBuffer()[8] == '!' ) && (interface->packetSize() >= 171)) {
        int k;
        uint8_t* p = (uint8_t*) esp_config;
        for(k=0; k<171; k++) {
          p[k] = interface->packetBuffer()[k]; //copy packet to config
        }
        esp_config->opcode = 0;
        EEPROM.write(8,0);  //opcode is zero for data (also sets dirty flag)
        EEPROM.commit();
        reply = 1;
        Serial.print("eprom written, ");
      } else {
        Serial.println("packet error.");
      }
      if ( reply) {
        erasePassword(esp_config);                  //don't show pwd if queried
        wUDP.beginPacket(wUDP.remoteIP(), interface->dmxPort());
        wUDP.write((uint8_t*)esp_config, sizeof(*esp_config));
        wUDP.endPacket();
        Serial.println("reply complete.");
      }
      interface->packetBuffer()[0] = 0; //insure loop without recv doesn't re-trgger
    } // packet has ESP-DMX header
  }   // not good_dmx
}
