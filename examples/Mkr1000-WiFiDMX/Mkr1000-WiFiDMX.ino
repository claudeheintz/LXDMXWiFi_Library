/**************************************************************************/
/*!
    @file     MKR1000-WiFiDMX.ino
    @author   Claude Heintz
    @license  BSD (see LXDMXWiFi.h)
    @copyright 2016 by Claude Heintz All Rights Reserved

    Example using LXDMXWiFi_Library for output of Art-Net or E1.31 sACN from
    MKR1000 WiFi connection to serial DMX.  Or, input from DMX to the network.
    Allows remote configuration of WiFi connection and protocol settings.
    
    Art-Net(TM) Designed by and Copyright Artistic Licence (UK) Ltd
    sACN E 1.31 is a public standard published by the PLASA technical standards program
    
    NOTE:  For LXDMXWiFi library to compile along with the WiFi101 library
           Comment out lines 40 and 41 in LXDMXWiFi.h
           
           This example requires the LXSAMD21DMX library for DMX serial output
           https://github.com/claudeheintz/LXSAMD21DMX

           As of this version, remote config is supported but because SAM D-21 has no EEPROM
           configuration is not persistent.
           So, it is necessary to edit initConfig in LXDMXWiFiConfig.h (second tab)
           and to re-upload the sketch to change WiFi and/or protocol settings.

           As of this version of WiFi101_Library, In access point mode, the MKR1000 does not
           receive broadcast or multicast UDP packets.  So, you must unicast to 192.168.1.1.

    @section  HISTORY

    v1.0 - First release

*/
/**************************************************************************/
#include <LXSAMD21DMX.h>
#include <WiFi101.h>
#include <WiFiUdp.h>
#include "LXDMXWiFi.h"
#include <LXWiFiArtNet.h>
#include <LXWiFiSACN.h>
#include "LXDMXWiFiConfig.h"

#define STARTUP_MODE_PIN 16      // pin for force default setup when low (use 10k pullup to insure high)
#define DIRECTION_PIN 3          // pin for output direction enable on MAX481 chip
#define LED_PIN 6                // MKR1000 has led on pin 6

/*         
 *  Edit the initConfig function in LXDMXWiFiConfig.h to configure the WiFi connection and protocol options
 */
DMXWiFiConfig* wifi_config = new ( DMXWiFiConfig );

// dmx protocol interface for parsing packets (created in setup)
LXDMXWiFi* interface;

// An EthernetUDP instance to let us send and receive UDP packets
WiFiUDP wUDP;

// direction output from network/input to network
uint8_t dmx_direction = 0;

// received slots when inputting dmx to network
int got_dmx = 0;

/* 
   utility function to toggle indicator LED on/off
*/
uint8_t led_state = 0;

void blinkLED() {
  if ( led_state ) {
    digitalWrite(LED_PIN, HIGH);
    led_state = 0;
  } else {
    digitalWrite(LED_PIN, LOW);
    led_state = 1;
  }
}

/* 
   artAddress callback allows storing of config information
   artAddress may or may not have set this information
   but relevant fields are copied to config struct and stored to EEPROM
*/
void artAddressReceived() {
  int u = ((LXWiFiArtNet*)interface)->universe();
  wifi_config->artnet_universe = u & 0x0F;
  wifi_config->artnet_subnet = (u>>4) & 0x0F;
  strncpy((char*)wifi_config->node_name, ((LXWiFiArtNet*)interface)->longName(), 31);
  wifi_config->node_name[32] = 0;
  wifi_config->opcode = 0;

}

/*
  DMX input callback function sets number of slots received by SAMD21DMX
*/
void gotDMXCallback(int slots) {
  got_dmx = slots;
}

/************************************************************************

  Setup creates the WiFi connection.
  
  It also creates the network protocol object,
  either an instance of LXWiFiArtNet or LXWiFiSACN.
  
  if OUTPUT_FROM_NETWORK_MODE:
     Starts listening on the appropriate UDP port.
  
     And, it starts the SAMD21DMX sending serial DMX via the UART1 TX pin.
     (see the SAMD21DMX library documentation for driver details)
     
   if INPUT_TO_NETWORK_MODE:
     Starts SAMD21DMX listening for DMX ( received as serial on UART0 RX pin. )

*************************************************************************/

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600); //debug messages

  //initialize configuration
  initConfig(wifi_config);

  int wifi_status = WL_IDLE_STATUS;
  if ( wifi_config->wifi_mode == AP_MODE ) {            // WiFi startup
    while (wifi_status == WL_IDLE_STATUS) {            // Access Point mode
      wifi_status = WiFi.beginAP(wifi_config->ssid);
      int blink_count = 0;
      while ((wifi_status == WL_IDLE_STATUS) && ( blink_count < 25)) {
        blinkLED();
        blink_count++;
        delay(200);
        wifi_status = WiFi.status();
      }
    }
    if ( wifi_config->protocol_mode & STATIC_MODE ) {  
      WiFi.config(wifi_config->ap_address, (uint32_t)0, wifi_config->ap_gateway, wifi_config->ap_subnet);
    }
    IPAddress ip = WiFi.localIP();
    Serial.print("Access Point IP Address: ");
    Serial.println(ip);
  } else {                                             // Station Mode
    while (wifi_status == WL_IDLE_STATUS) {            // Access Point mode
      wifi_status = WiFi.begin(wifi_config->ssid, wifi_config->pwd);
      int blink_count = 0;
      while ((wifi_status == WL_IDLE_STATUS) && ( blink_count < 25)) {
        blinkLED();
        blink_count++;
        delay(200);
        wifi_status = WiFi.status();
      }
    }

    if ( wifi_config->protocol_mode & STATIC_MODE ) {  
      WiFi.config(wifi_config->sta_address, (uint32_t)0, wifi_config->sta_gateway, wifi_config->sta_subnet);
    }

    IPAddress ip = WiFi.localIP();
    Serial.print("Station IP Address: ");
    Serial.println(ip);   
  }

  if ( wifi_config->protocol_mode & SACN_MODE ) {         // Initialize network<->DMX interface
    interface = new LXWiFiSACN();
    interface->setUniverse(wifi_config->sacn_universe);
  } else {
    interface = new LXWiFiArtNet(WiFi.localIP(), WiFi.subnetMask());
    ((LXWiFiArtNet*)interface)->setSubnetUniverse(wifi_config->artnet_subnet, wifi_config->artnet_universe);
    ((LXWiFiArtNet*)interface)->setArtAddressReceivedCallback(&artAddressReceived);
    if ( wifi_config->node_name[0] != 0 ) {
      wifi_config->node_name[32] = 0;      //insure zero termination of string
      strcpy(((LXWiFiArtNet*)interface)->longName(), (char*)wifi_config->node_name);
    }
  }
  Serial.print("interface created;");

  dmx_direction = ( wifi_config->protocol_mode & INPUT_TO_NETWORK_MODE );
  
  // if OUTPUT from network, start wUDP listening for packets
  if ( dmx_direction == OUTPUT_FROM_NETWORK_MODE ) {  
    if ( ( wifi_config->protocol_mode & MULTICAST_MODE ) ) { // Start listening for UDP on port
      wUDP.beginMulti(wifi_config->multi_address, interface->dmxPort());
    } else {
      wUDP.begin(interface->dmxPort());
    }
    Serial.print("udp listening started;");

    if ( ( wifi_config->protocol_mode & SACN_MODE ) == 0 ) { //if needed, announce presence via Art-Net Poll Reply
      ((LXWiFiArtNet*)interface)->send_art_poll_reply(&wUDP);
    }

    Serial.print("starting DMX output;");
    SAMD21DMX.setDirectionPin(DIRECTION_PIN);
    SAMD21DMX.startOutput();
  } else {                    //direction is INPUT to network
    Serial.print("starting DMX input;");
    SAMD21DMX.setDirectionPin(DIRECTION_PIN);
    SAMD21DMX.setDataReceivedCallback(&gotDMXCallback);
    SAMD21DMX.startInput();
  }

  Serial.println("setup complete.");
}

/************************************************************************

  Main loop
  
  if OUTPUT_FROM_NETWORK_MODE:
    checks for and reads packets from WiFi UDP socket
    connection.  readDMXPacket() returns true when a DMX packet is received.
    In which case, the data is copied to the SAMD21DMX object which is driving
    the UART serial DMX output.
  
    If the packet is an CONFIG_PACKET_IDENT packet, the config struct is modified and stored in EEPROM
  
  if INPUT_TO_NETWORK_MODE:
    if serial dmx has been received, sends an sACN or Art-Net packet containing the dmx data.
    Note:  does not listen for incoming packets for remote configuration in this mode.

*************************************************************************/

void loop() {
  
  if ( dmx_direction == OUTPUT_FROM_NETWORK_MODE ) {
    uint8_t good_dmx = interface->readDMXPacket(&wUDP);
  
    if ( good_dmx ) {
       Serial.print("dmx ");
       for (int i = 1; i <= interface->numberOfSlots(); i++) {
          SAMD21DMX.setSlot(i , interface->getSlot(i));
       }
       blinkLED();
    } else {
      if ( strcmp(CONFIG_PACKET_IDENT, (const char *) interface->packetBuffer()) == 0 ) {  //match header to config packet
        Serial.print("config packet received, ");
        uint8_t reply = 0;
        if ( interface->packetBuffer()[8] == '?' ) {  //packet opcode is query
          
          reply = 1;
        } else if (( interface->packetBuffer()[8] == '!' ) && (interface->packetSize() >= 171)) { //packet opcode is set
          int k;
          uint8_t* p = (uint8_t*) wifi_config;
          for(k=0; k<171; k++) {
           p[k] = interface->packetBuffer()[k]; //copy packet to config
          }
          wifi_config->opcode = 0;               // reply packet opcode is data
          if (interface->packetSize() >= 203) {
           for(k=171; k<interface->packetSize(); k++) {
            p[k] = interface->packetBuffer()[k]; //copy rest of packet
           }
           strcpy(((LXWiFiArtNet*)interface)->longName(), (char*)&interface->packetBuffer()[171]);
          }
  
          reply = 1;
          Serial.print("eprom written, ");
        } else {
          Serial.println("packet error.");
        }
        if ( reply) {
          erasePassword(wifi_config);                  //don't show pwd if queried
          wUDP.beginPacket(wUDP.remoteIP(), interface->dmxPort());
          wUDP.write((uint8_t*)wifi_config, sizeof(*wifi_config));
          wUDP.endPacket();
          Serial.println("reply complete.");
        }
        interface->packetBuffer()[0] = 0; //insure loop without recv doesn't re-trgger
        blinkLED();
        delay(100);
        blinkLED();
        delay(100);
        blinkLED();
      }     // packet has config packet header
    }       // not good_dmx
    
  } else {    //direction is input to network
    
    if ( got_dmx ) {
      interface->setNumberOfSlots(got_dmx);  //got_dmx
     
      for(int i=1; i<=got_dmx; i++) {
        interface->setSlot(i, SAMD21DMX.getSlot(i));
      }
     
      if ( wifi_config->protocol_mode & MULTICAST_MODE ) {
        interface->sendDMX(&wUDP, wifi_config->input_address, WiFi.localIP());
      } else {
        interface->sendDMX(&wUDP, wifi_config->input_address, INADDR_NONE);
      }
      
      got_dmx = 0;
      blinkLED();
      
    }       // got_dmx
    
  }         // INPUT_TO_NETWORK_MODE
  
}           // loop()
