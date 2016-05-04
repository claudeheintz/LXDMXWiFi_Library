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
    
    NOTE:  If not using the latest Arduino IDE, you may get compile errors when including the WiFi101 library.
              If you get the following error (and others):
              "error: 'SOCKET' does not name a type SOCKET _socket;"
              Comment out lines 40 and 41 in LXDMXWiFi.h
           
           This example requires the LXSAMD21DMX library for DMX serial output
           https://github.com/claudeheintz/LXSAMD21DMX

           As of this version, remote config is supported using the configuration
           utility found in the examples folder

           As of this version of WiFi101_Library, In access point mode, the MKR1000 does not
           receive broadcast or multicast UDP packets.  So, you must unicast to 192.168.1.1.

    @section  HISTORY

    v1.0 - First release
    v1.1 - Adds persistence

*/
/**************************************************************************/
#include <LXSAMD21DMX.h>
#include <WiFi101.h>
#include <WiFiUdp.h>
#include "LXDMXWiFi.h"
#include <LXWiFiArtNet.h>
#include <LXWiFiSACN.h>
#include "LXDMXWiFiConfig.h"

#define STARTUP_MODE_PIN 7      // pin for force default setup when low (use 10k pullup to insure high)
#define DIRECTION_PIN 3          // pin for output direction enable on MAX481 chip
#define LED_PIN 6                // MKR1000 has led on pin 6

/*         
 *  Edit the LXDMXWiFiConfig.initConfig() function in LXDMXWiFiConfig.cpp to configure the WiFi connection and protocol options
 */

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
   but relevant fields are copied to config struct (and stored to EEPROM ...not yet)
*/
void artAddressReceived() {
  DMXWiFiConfig.setArtNetUniverse( ((LXWiFiArtNet*)interface)->universe() );
  DMXWiFiConfig.setNodeName( ((LXWiFiArtNet*)interface)->longName() );
  DMXWiFiConfig.commitToPersistentStore();
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
  pinMode(STARTUP_MODE_PIN, INPUT);
 // while ( ! Serial ) {}     //force wait for serial connection.  Sketch will not continue until Serial Monitor is opened.
  Serial.begin(9600);         //debug messages
  Serial.println("_setup_");
  
  DMXWiFiConfig.begin(digitalRead(STARTUP_MODE_PIN));

  int wifi_status = WL_IDLE_STATUS;
  if ( DMXWiFiConfig.APMode() ) {                      // WiFi startup
    while (wifi_status == WL_IDLE_STATUS) {            // Access Point mode
      wifi_status = WiFi.beginAP(DMXWiFiConfig.SSID());
      int blink_count = 0;
      while ((wifi_status == WL_IDLE_STATUS) && ( blink_count < 25)) {
        blinkLED();
        blink_count++;
        delay(200);
        wifi_status = WiFi.status();
      }
    }
    if ( DMXWiFiConfig.staticIPAddress() ) {  
      WiFi.config(DMXWiFiConfig.apIPAddress(), (uint32_t)0, DMXWiFiConfig.apGateway(), DMXWiFiConfig.apSubnet());
    }
    Serial.print("Access Point IP Address: ");
  } else {                                             // Station Mode
    while (wifi_status == WL_IDLE_STATUS) {
      wifi_status = WiFi.begin(DMXWiFiConfig.SSID(), DMXWiFiConfig.password());
      int blink_count = 0;
      while ((wifi_status == WL_IDLE_STATUS) && ( blink_count < 25)) {
        blinkLED();
        blink_count++;
        delay(200);
        wifi_status = WiFi.status();
      }
      Serial.print("Station IP Address: ");
    }

    if ( DMXWiFiConfig.staticIPAddress() ) {  
      WiFi.config(DMXWiFiConfig.stationIPAddress(), (uint32_t)0, DMXWiFiConfig.stationGateway(), DMXWiFiConfig.stationSubnet());
    }
     
  }
  
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);

  if ( DMXWiFiConfig.sACNMode() ) {         // Initialize network<->DMX interface
    interface = new LXWiFiSACN();
    interface->setUniverse(DMXWiFiConfig.sACNUniverse());
  } else {
    interface = new LXWiFiArtNet(WiFi.localIP(), WiFi.subnetMask());
    ((LXWiFiArtNet*)interface)->setSubnetUniverse(DMXWiFiConfig.artnetSubnet(), DMXWiFiConfig.artnetUniverse());
    ((LXWiFiArtNet*)interface)->setArtAddressReceivedCallback(&artAddressReceived);
    char* nn = DMXWiFiConfig.nodeName();
    if ( nn[0] != 0 ) {
      strcpy(((LXWiFiArtNet*)interface)->longName(), nn);
    }
  }
  Serial.print("interface created;");

  dmx_direction = ( DMXWiFiConfig.inputToNetworkMode() );
  
  // if OUTPUT from network, start wUDP listening for packets
  if ( dmx_direction == OUTPUT_FROM_NETWORK_MODE ) {  
    if ( ( DMXWiFiConfig.multicastMode() ) ) { // Start listening for UDP on port
      wUDP.beginMulti(DMXWiFiConfig.multicastAddress(), interface->dmxPort());
    } else {
      wUDP.begin(interface->dmxPort());
    }
    Serial.print("udp listening started;");

    if ( DMXWiFiConfig.artnetMode() ) { //if needed, announce presence via Art-Net Poll Reply
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
       for (int i = 1; i <= interface->numberOfSlots(); i++) {
          SAMD21DMX.setSlot(i , interface->getSlot(i));
       }
       blinkLED();
    } else {
      if ( strcmp(CONFIG_PACKET_IDENT, (const char *) interface->packetBuffer()) == 0 ) {  //match header to config packet
        Serial.print("config packet received, ");
        uint8_t reply = 0;
        if ( interface->packetBuffer()[8] == '?' ) {  //packet opcode is query
          DMXWiFiConfig.readFromPersistentStore();
          reply = 1;
        } else if (( interface->packetBuffer()[8] == '!' ) && (interface->packetSize() >= 171)) { //packet opcode is set
          Serial.println("upload packet");
          DMXWiFiConfig.copyConfig( interface->packetBuffer(), interface->packetSize());
          DMXWiFiConfig.commitToPersistentStore();
          reply = 1;
        } else {
          Serial.println("packet error.");
        }
        if ( reply) {
          DMXWiFiConfig.hidePassword();                  //don't transmit password!
          wUDP.beginPacket(wUDP.remoteIP(), interface->dmxPort());
          wUDP.write((uint8_t*)DMXWiFiConfig.config(), DMXWiFiConfigSIZE);
          wUDP.endPacket();
          Serial.println("reply complete.");
			  DMXWiFiConfig.restorePassword();
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
     
      if ( DMXWiFiConfig.multicastMode() ) {
        interface->sendDMX(&wUDP, DMXWiFiConfig.inputAddress(), WiFi.localIP());
      } else {
        interface->sendDMX(&wUDP, DMXWiFiConfig.inputAddress(), INADDR_NONE);
      }
      
      got_dmx = 0;
      blinkLED();
      
    }       // got_dmx
    
  }         // INPUT_TO_NETWORK_MODE
  
}           // loop()
