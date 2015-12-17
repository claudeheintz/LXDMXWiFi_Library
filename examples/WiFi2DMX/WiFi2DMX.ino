/**************************************************************************/
/*!
    @file     WiFi2DMX.ino
    @author   Claude Heintz
    @license  BSD (see LXDMXWiFi.h)
    @copyright 2015 by Claude Heintz All Rights Reserved

    Example using LXEDMXWiFi_Library for output of Art-Net or E1.31 sACN from
    ESP 8266 WiFi connection to DMX
    
    Art-Net(TM) Designed by and Copyright Artistic Licence (UK) Ltd
    sACN E 1.31 is a public standard published by the PLASA technical standards program
    
    Note:  For sending packets larger than 512 bytes, ESP8266 WiFi Library v2.1
           is necessary.
           
           This example requires the LXESP8266DMX library for DMX serial output
           https://github.com/claudeheintz/LXESP8266DMX

    @section  HISTORY

    v1.0 - First release
*/
/**************************************************************************/

#include <LXESP8266UARTDMX.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <LXDMXWiFi.h>
#include <LXWiFiArtNet.h>
#include <LXWiFiSACN.h>

const char* ssid = "ESP2DMX";
const char* pwd = "***";
uint8_t make_access_point = 1;
uint8_t chan = 2;

uint8_t use_sacn = 1;
uint8_t use_multicast = 1;

// dmx protocol interface for parsing packets (created in setup)
LXDMXWiFi* interface;

// LX8266DMXOutput instance for DMX output using UART1/GPIO2
LX8266DMXOutput* dmx_output = new LX8266DMXOutput();

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

void setup() {
  Serial.begin(112500);
  Serial.setDebugOutput(1); //use uart0 for debugging
  pinMode(BUILTIN_LED, OUTPUT);

  if ( make_access_point ) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid);
    WiFi.softAPConfig(IPAddress(10,110,115,10), IPAddress(10,1,1,1), IPAddress(255,0,0,0));
  } else {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid,pwd);
    WiFi.config(IPAddress(10,110,115,15), IPAddress(192,168,1,1), IPAddress(255,0,0,0));

    while (WiFi.status() != WL_CONNECTED) {
      delay(100);
      blinkLED();
    }
  }

  if ( use_sacn ) {                  // Initialize Interface
    interface = new LXWiFiSACN();
  } else {
    interface = new LXWiFiArtNet(WiFi.localIP(), WiFi.subnetMask());
    use_multicast = 0;
  }

  if ( use_multicast ) {  // Start listening for UDP on port
    if ( make_access_point ) {
      wUDP.beginMulticast(WiFi.softAPIP(), IPAddress(239,255,0,1), interface->dmxPort());
    } else {
      wUDP.beginMulticast(WiFi.localIP(), IPAddress(239,255,0,1), interface->dmxPort());
    }
  } else {
    wUDP.begin(interface->dmxPort());
  }

  //announce presence via Art-Net Poll Reply
  if ( ! use_sacn ) {
     ((LXWiFiArtNet*)interface)->send_art_poll_reply(wUDP);
  }

  dmx_output->start();

  Serial.println("\nsetup complete");
  blinkLED();
}

/************************************************************************

  The main loop checks for and reads packets from UDP ethernet socket
  connection.  When a packet is recieved, it is checked to see if
  it is valid Art-Net and the art DMXReceived function is called, sending
  the DMX values to the output.

*************************************************************************/

void loop() {
  uint8_t good_dmx = interface->readDMXPacket(wUDP);

  if ( good_dmx ) {
     for (int i = 1; i <= interface->numberOfSlots(); i++) {
        dmx_output->setSlot(i , interface->getSlot(i));
     }
     blinkLED();
  }
}
