/**************************************************************************/
/*!
    @file     WioTerminalArtNet.ino
    @author   Claude Heintz
    @license  BSD (see LXDMXWiFi.h)
    @copyright 2020 by Claude Heintz All Rights Reserved

    This sketch is an example of Art-Net to DMX on a Seeed Wio Terminal.
    Art-Net is received over a WiFi connection an read using LXDMXWiFi_Library.
    Serial DMX is output using LXSAMD51DMX library.

    The WiFi connection and Art-Net settings are configured using access point mode and a web browser.
    The configuration settings are stored in a file on an SD card.
    
        - Note that this file will contain the WiFi password in plain text.
        - When no file exists on the SD card, Access Point configuration mode is automatically entered.
        - When no SD card exists, the default WiFi SSID and password
           defined in wifi_connection_info.h are used.
        - Holding down the "A" button on top of the Wio Terminal on boot forces configuration mode.
        - To configure, connect a computer to the Access Point WiFi SSID: "WioTerminalSetup".
           Then, point a browser to 192.168.1.1
        - The settings are saved to SD card when Submit is clicked.
        - Reboot after configuring to enter normal operating mode:
           The Wio Terminal will connect to the configured WiFi network.
    
    Art-Net(TM) Designed by and Copyright Artistic Licence Holdings Ltd.
    
    NOTES: See https://wiki.seeedstudio.com/Wio-Terminal-Network-Overview/ for setting up Realtek WiFi firmware
               --required for #include <rpcWiFi.h>
               --WioTerminal connection based on Seeed Arduino WiFi, WiFiUDPClient example.
               
           See https://wiki.seeedstudio.com/Wio-Terminal-FS-Overview/ for SD card library
               --required for #include <Seeed_FS.h>, #include "SD/Seeed_SD.h"
           
           This example requires the LXSAMD51DMX library for DMX serial output
           https://github.com/claudeheintz/LXSAMD51DMX

    @section  HISTORY

    v1.0 - First release
    
*/
/**************************************************************************/
#include <rpcWiFi.h>
#include <WiFiAP.h>
#include <WiFiUdp.h>
#include "LXDMXWiFi.h"
#include <LXWiFiArtNet.h>
#include <LXSAMD51DMX.h>
#include <TFT_eSPI.h>


// uncomment to enable Serial
//#define ENABLE_DEBUG_PRINT 1

// WiFi network name and password are defined in:
#include "wifi_connection_info.h"

//Are we currently connected?
boolean connected = false;
boolean config_mode = false;

// The Art-Net interface and result of readDMXPacket
LXWiFiArtNet* artNetInterface = NULL;
int art_packet_result = 0;

//The udp library class
WiFiUDP udp;

// display
TFT_eSPI tft;
int tftln = 10;

#define DIRECTION_PIN 2
#define BUILTIN_LED 13
uint8_t led_state = 0;

void setup(){
  // init pins
  pinMode(WIO_KEY_A, INPUT_PULLUP);
  config_mode = ( digitalRead(WIO_KEY_A) == LOW );
  pinMode(BUILTIN_LED, OUTPUT);

  // init serial 
 #ifdef ENABLE_DEBUG_PRINT
  // Initilize hardware serial:
  Serial.begin(115200);
  while ( ! Serial ) {}
 #endif
  initDisplay();

  int result = init_connection_info();
  if ( result == -2 ) {
    displayMessage("No SD card!");
    displayMessage("Using default connection");
    config_mode = false;
  } else if ( result == -1 ) {
    displayMessage("Could not open or read");
    displayMessage("wioterminal.wificonfig");
    config_mode = true;
  }

  if ( config_mode ) {
    // initialize Access Point & server
    init_access_point_and_server();
    displayMessage("Configure Mode!");
    displayMessage("Connect computer WiFi to:");
    displayMessage(CONFIG_AP_SSID);
    displayMessage("Enter in browser:");
    displayMessage(WiFi.localIP());
  } else {
    //Connect to the WiFi network
    connectToWiFi(wifi_ssid(), wifi_password());

    SAMD51DMX.setDirectionPin(DIRECTION_PIN); //or wire !RE & DE pins on Max485 to HIGH
    SAMD51DMX.startOutput();
  }
}

void loop(){
  if ( connected ) {
    // Calls UDP.parsePacket to read from network, if available.
    // returns RESULT_DMX_RECEIVED if ArtDMX packet was read
    // The library automatically responds to ArtPoll packets with ArtPollReplies
    art_packet_result = artNetInterface->readDMXPacket(&udp);   

    if (art_packet_result == RESULT_DMX_RECEIVED) {
      copyDMXToOutput();
      blinkLED();
    }
  } else if ( config_mode ) {
    check_server();
  }
}

/***********************************************************
 *  Wifi Connection Functions
 ***********************************************************/

void connectToWiFi(const char * ssid, const char * pwd){
  displayMessage("Connecting to WiFi network:");
  displayMessage(ssid);

  // delete old config
  WiFi.disconnect(true);
  //register event handler
  WiFi.onEvent(WiFiEvent);
  
  //Initiate connection
  WiFi.begin(ssid, pwd);  // does not seem that WioTerminal receives UDP in AP mode (?)
}

//wifi event handler
void WiFiEvent(WiFiEvent_t event){
    switch(event) {
      case SYSTEM_EVENT_STA_GOT_IP:
      case SYSTEM_EVENT_AP_START:
          //When connected set 
          displayMessage("WiFi connected! IP address: ");
          displayMessage(WiFi.localIP());
          
          //initializes the UDP state
          if ( ! connected ) {
            if ( artNetInterface == NULL ) {
              artNetInterface = new LXWiFiArtNet(WiFi.localIP(), WiFi.subnetMask());
              // announce availability to network
              strncpy(artNetInterface->longName(), node_name(), NODE_NAME_MAX_LENGTH);
              strncpy(artNetInterface->shortName(), node_id(), NODE_ID_MAX_LENGTH);
              artNetInterface->setUniverse(universe());
              artNetInterface->send_art_poll_reply(&udp);
              artNetInterface->setArtAddressReceivedCallback(&artAddressReceived);
            }
            
            udp.begin(WiFi.localIP(), ARTNET_PORT);
            connected = true;
          }
          break;
      case SYSTEM_EVENT_STA_DISCONNECTED:
          displayMessage("WiFi lost connection");
          connected = false;
          delay(3000);
          displayMessage("re-try...");
          WiFi.disconnect(true);
          WiFi.begin(wifi_ssid(), wifi_password());
          break;
      case SYSTEM_EVENT_AP_STACONNECTED:
          displayMessage("A station connected to AP: ");
          displayMessage(WiFi.localIP()); 
      default: break;
    }
}

/***********************************************************
 *  Display Functions
 ***********************************************************/

 void initDisplay() {
  // UI Basic Layout
  tft.begin();
  tft.setRotation(3);
  eraseDisplay();

  tft.setTextColor(TFT_PINK);
  tft.setTextSize(2);
  displayMessage("Wio Terminal");;
  displayMessage("Art-Net -> DMX Interface");
  tft.setTextColor(TFT_WHITE);
 }

 void eraseDisplay() {
  // screen is 320x240
  tft.fillRect(0, 0, 320, 240, TFT_DARKGREY);
 }

 void displayMessage(char* str) {
  if ( tftln > 220 ) {
    eraseDisplay();
    tftln = 10;
  }
  tft.drawString(str, 3, tftln);
  tftln += 17;
 }

  void displayMessage(const char* str) {
    displayMessage((char*) str);
  }

  void displayMessage(IPAddress ip) {
    displayMessage(ip.toString().c_str());
  }

/*************************************************
   utility function to toggle indicator LED on/off
*/
void blinkLED() {
  if ( led_state ) {
    digitalWrite(BUILTIN_LED, HIGH);
    led_state = 0;
  } else {
    digitalWrite(BUILTIN_LED, LOW);
    led_state = 1;
  }
}

/***********************************************************
 *  Art-Net Implementation Functions
 ***********************************************************/

void copyDMXToOutput(void) {
  uint8_t a;
  uint16_t a_slots = artNetInterface->numberOfSlots();
  for (int i=1; i <=DMX_UNIVERSE_SIZE; i++) {
    if ( i <= a_slots ) {
      a = artNetInterface->getSlot(i);
    } else {
      a = 0;
    }
    SAMD51DMX.setSlot(i , a);
  }
}

void artAddressReceived() {
  setUniverse( artNetInterface->universe() );
  strncpy( node_name(), artNetInterface->longName(), NODE_NAME_MAX_LENGTH);
  strncpy( node_id(), artNetInterface->shortName(), NODE_ID_MAX_LENGTH);
  saveSettings();
}
