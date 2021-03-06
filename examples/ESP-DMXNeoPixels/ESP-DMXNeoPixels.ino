/**************************************************************************/
/*!
    @file     ESP-DMXNeoPixels.ino
    @author   Claude Heintz
    @license  BSD (see LXDMXWiFi.h)
    @copyright 2016-2021 by Claude Heintz All Rights Reserved

    Example using LXDMXWiFi_Library for output of Art-Net or E1.31 sACN from
    ESP8266 Adafruit Huzzah WiFi connection to an Adafruit NeoPixel Ring.
    
    Art-Net(TM) Designed by and Copyright Artistic Licence Holdings Ltd.
    sACN E 1.31 is a public standard published by the PLASA technical standards program
    
    NOTE:  This example requires the Adafruit NeoPixel Library and WiFi101 Library

           Remote config is supported using the configuration utility in the examples folder.
           Otherwise edit initConfig() in LXDMXWiFiConfig.cpp.

    @section  HISTORY

    v1.0 - First release
    v1.1 - Add USE_REMOTE_CONFIG option
    v1.2 - Refactor order of utility functions in main sketch,
           move WiFi connection setup to LXDMXWiFiConfig
           move checkConfigReceived to LXDMXWiFiConfig

*/
/**************************************************************************/

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "LXDMXWiFi.h"
#include <LXWiFiArtNet.h>
#include <LXWiFiSACN.h>
#include "LXDMXWiFiConfig.h"

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define STARTUP_MODE_PIN 16      // pin for force default setup when low (if not input_pullup capable, use 10k pullup to insure high)
#define LED_PIN BUILTIN_LED

// data pin for NeoPixels
#define PIN 14
// NUM_OF_NEOPIXELS, max of 170/RGB or 128/RGBW
#define NUM_OF_NEOPIXELS 12
// LEDS_PER_NEOPIXEL, RGB = 3, RGBW = 4
#define LEDS_PER_NEOPIXEL 3
// see Adafruit NeoPixel Library for options to pass to Adafruit_NeoPixel constructor
Adafruit_NeoPixel ring = Adafruit_NeoPixel(NUM_OF_NEOPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// min sometimes raises compile error. see https://github.com/esp8266/Arduino/issues/263 solution seems to be using _min
const int total_pixels = min(512, LEDS_PER_NEOPIXEL * NUM_OF_NEOPIXELS);
byte pixels[NUM_OF_NEOPIXELS][LEDS_PER_NEOPIXEL];

/*         
 *  To allow use of the configuration utility, uncomment the following statement
 *  to define USE_REMOTE_CONFIG.
 *
 *  When using remote configuration:
 *        The remote configuration utility can be used to edit the settings
 *        without re-loading the sketch.  Settings from persistent memory are
 *        used unless the startup pin is read LOW.  Holding the startup pin low
 *        temporarily uses the settings in the LXDMXWiFiConfig.initConfig() method.
 *        This insures there is a default way of connecting to the sketch
 *        in order to use the remote utility, even if it is configured to use
 *        a WiFi network that is unavailable.
 *
 *	Without remote configuration (USE_REMOTE_CONFIG remains undefined):
 *        Settings are read from the LXDMXWiFiConfig.initConfig() method.
 *        So, it is necessary to edit that function in order to change
 *        the settings.
 *
 *  Important for using remote configuration:
 *        It is necessary to initialize the boards EEPROM flash with the
 *        following steps.
 *
 *       ------------------------------------------------
 *        1) In ESP-DMX.ino line 83 should read:
 *           #define USE_REMOTE_CONFIG 0
 *
 *        2) In LXDMXWiFiConfig.cpp uncomment line 35:
 *           #define RESET_PERSISTENT_CONFIG_ON_DEFAULT 1
 *
 *        3) Flash the ESP8266 and reboot.
 *
 *        4) In LXDMXWiFiConfig.cpp comment out line 35:
 *           //#define RESET_PERSISTENT_CONFIG_ON_DEFAULT 1
 *
 *        5) Flash the ESP8266 and reboot.
*       --------------------------------------------------
 */
//#define USE_REMOTE_CONFIG 0

// dmx protocol interfaces for parsing packets (created in setup)
LXWiFiArtNet* artNetInterface;
LXWiFiSACN*   sACNInterface;

// EthernetUDP instances to let us send and receive UDP packets
WiFiUDP aUDP;
WiFiUDP sUDP;

// direction output from network/input to network
uint8_t dmx_direction = 0;

// Output mode: received packet contained dmx
int art_packet_result = 0;
int acn_packet_result = 0;



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
  pinMode(STARTUP_MODE_PIN, INPUT_PULLUP);
 // while ( ! Serial ) {}     //force wait for serial connection.  Sketch will not continue until Serial Monitor is opened.
  Serial.begin(115200);       //debug messages
  Serial.println("_setup_");

#ifdef USE_REMOTE_CONFIG
  DMXWiFiConfig.begin(digitalRead(STARTUP_MODE_PIN));	// uses settings from persistent memory unless pin is low.
#else
  DMXWiFiConfig.begin();								// must edit DMXWiFiConfig.initConfig() to change settings
#endif


  if ( DMXWiFiConfig.setupWiFi(blinkLED) & CONFIG_AP_MODE ) {
    Serial.print("Access Point IP Address: ");
  } else {
    Serial.print("Station IP Address: ");
  }
  
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);

  //------------------- Initialize network<->DMX interfaces -------------------
    
  sACNInterface = new LXWiFiSACN();							
  sACNInterface->setUniverse(DMXWiFiConfig.sACNUniverse());

  artNetInterface = new LXWiFiArtNet(WiFi.localIP(), WiFi.subnetMask());
  artNetInterface->setUniverse(DMXWiFiConfig.artnetPortAddress());	//setUniverse for LXArtNet class sets complete Port-Address
  artNetInterface->setArtAddressReceivedCallback(&artAddressReceived);
  artNetInterface->setArtIpProgReceivedCallback(&artIpProgReceived);
  char* nn = DMXWiFiConfig.nodeName();
  if ( nn[0] != 0 ) {
    strcpy(artNetInterface->longName(), nn);
  }
  Serial.print("interfaces created, ");

  dmx_direction = ( DMXWiFiConfig.inputToNetworkMode() );
  
  // if output from network, start wUDP listening for packets
  if ( dmx_direction == OUTPUT_FROM_NETWORK_MODE ) {	
		if ( DMXWiFiConfig.multicastMode() ) {
			if ( DMXWiFiConfig.APMode() ) {
				sUDP.beginMulticast(WiFi.softAPIP(), DMXWiFiConfig.multicastAddress(), sACNInterface->dmxPort());
			} else {
				sUDP.beginMulticast(WiFi.localIP(), DMXWiFiConfig.multicastAddress(), sACNInterface->dmxPort());
			}
		} else {
			sUDP.begin(sACNInterface->dmxPort());
		}
    
		aUDP.begin(artNetInterface->dmxPort());
		artNetInterface->send_art_poll_reply(&aUDP);
		Serial.print("udp started listening, ");

		ring.begin();
		ring.show();
  } else {                    //direction is INPUT to network
    // doesn't do anything in this mode
  }
  
  Serial.println("setup complete.");
}

/************************************************************************

  Main loop
  
  if OUTPUT_FROM_NETWORK_MODE:
    checks for and reads packets from WiFi UDP socket
    connection.  readDMXPacket() returns true when a DMX packet is received.
    
    If dmx is received on either interface, copy from both (HTP) to dmx output.
  
    If the packet is an CONFIG_PACKET_IDENT packet, the config struct is modified and stored in EEPROM
  
  if INPUT_TO_NETWORK_MODE:
    if serial dmx has been received, sends an sACN or Art-Net packet containing the dmx data.
    Note:  does not listen for incoming packets for remote configuration in this mode.

*************************************************************************/

void loop() {
	if ( dmx_direction == OUTPUT_FROM_NETWORK_MODE ) {
	
		art_packet_result = artNetInterface->readDMXPacket(&aUDP);
		#ifdef USE_REMOTE_CONFIG
		if ( art_packet_result == RESULT_NONE ) {
			DMXWiFiConfig.checkConfigReceived(artNetInterface, aUDP, blinkLED, CONFIG_PRINT_MESSAGES);
		}
		#endif
		
		acn_packet_result = sACNInterface->readDMXPacket(&sUDP);
		#ifdef USE_REMOTE_CONFIG
		if ( acn_packet_result == RESULT_NONE ) {
			DMXWiFiConfig.checkConfigReceived(sACNInterface, sUDP, blinkLED, CONFIG_PRINT_MESSAGES);
		}
		#endif
		
		if ( (art_packet_result == RESULT_DMX_RECEIVED) || (acn_packet_result == RESULT_DMX_RECEIVED) ) {
			copyDMXToOutput();
			blinkLED();
		}
		
	} else {    //direction is input to network
	
		if ( DMXWiFiConfig.sACNMode() ) {
			checkInput(sACNInterface, &sUDP, DMXWiFiConfig.multicastMode());
		} else {
			checkInput(artNetInterface, &aUDP, 0);
		}
		
	}
}// loop()


/************************************************************************
   utility function to toggle indicator LED on/off
************************************************************************/
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

/************************************************************************
   artAddress callback allows storing of config information
   artAddress may or may not have set this information
   but relevant fields are copied to config struct (and stored to EEPROM ...not yet)
************************************************************************/
void artAddressReceived() {
  DMXWiFiConfig.setArtNetPortAddress( artNetInterface->universe() );
  DMXWiFiConfig.setNodeName( artNetInterface->longName() );
  DMXWiFiConfig.commitToPersistentStore();
}

/************************************************************************
   artIpProg callback allows storing of config information
   cmd field bit 7 indicates that settings should be programmed
************************************************************************/
void artIpProgReceived(uint8_t cmd, IPAddress addr, IPAddress subnet) {
   if ( cmd & 0x80 ) {
      if ( cmd & 0x40 ) {  //enable dhcp, other fields not written
        if ( DMXWiFiConfig.staticIPAddress() ) {
          DMXWiFiConfig.setStaticIPAddress(0);
        } else {
           return;  // already set to dhcp
        }
      } else {
         if ( ! DMXWiFiConfig.staticIPAddress() ) {
           DMXWiFiConfig.setStaticIPAddress(1); // static not dhcp
        }
        if ( cmd & 0x08 ) { //factory reset
           DMXWiFiConfig.initConfig();
        } else {
           if ( cmd & 0x04 ) {  //programIP
              DMXWiFiConfig.setStationIPAddress(addr);
           }
           if ( cmd & 0x02 ) {  //programSubnet
              DMXWiFiConfig.setStationSubnetMask(subnet);
           }
        }
      } // else ( ! dhcp )
      
      DMXWiFiConfig.commitToPersistentStore();
   }
}

/************************************************************************

  Copy to output merges slots for Art-Net and sACN on HTP basis
  
*************************************************************************/

void copyDMXToOutput(void) {
  uint8_t a, s;
  uint16_t a_slots = artNetInterface->numberOfSlots();
  uint16_t s_slots = sACNInterface->numberOfSlots();
  uint16_t low_addr = DMXWiFiConfig.deviceAddress();
  uint16_t high_addr = DMXWiFiConfig.deviceAddress()+total_pixels;
  for (int i=low_addr; i<high_addr; i++) {
    if ( i <= a_slots ) {
      a = artNetInterface->getSlot(i);
    } else {
      a = 0;
    }
    if ( i <= s_slots ) {
      s = sACNInterface->getSlot(i);
    } else {
      s = 0;
    }
    if ( a > s ) {
      setPixel(i-low_addr, a);
    } else {
      setPixel(i-low_addr, s);
    }
   }
   sendPixels();
}

/************************************************************************

  Checks to see if the dmx callback indicates received dmx
     If so, send it using the selected interface.
  
*************************************************************************/

void checkInput(LXDMXWiFi* interface, WiFiUDP* iUDP, uint8_t multicast) {
  // no input for this sketch
}

/************************************************************************
  sends pixel buffer to ring after applying gamma correction
************************************************************************/

void sendPixels() {
  uint16_t r,g,b;
  for (int p=0; p<NUM_OF_NEOPIXELS; p++) {
    r = pixels[p][0];
    g = pixels[p][1];
    b = pixels[p][2];
    r = (r*r)/255;    //gamma correct
    g = (g*g)/255;
    b = (b*b)/255;
    if ( LEDS_PER_NEOPIXEL == 3 ) {
      ring.setPixelColor(p, r, g, b);   //RGB
    } else if ( LEDS_PER_NEOPIXEL == 4 ) {
      uint16_t w = pixels[p][3];
      w = (w*w)/255;
      ring.setPixelColor(p, r, g, b, w);  //RGBW
    }
  }
  ring.show();
}

/************************************************************************
  sets the value of individual byte in the pixel buffer
  Assumes that linear index (ie DMX slot address) corresponds to expansion of [pixel][color]
************************************************************************/

void setPixel(uint16_t index, uint8_t value) {  
  uint8_t pixel = index/LEDS_PER_NEOPIXEL;
  uint8_t color = index%LEDS_PER_NEOPIXEL;
  pixels[pixel][color] = value;
}
