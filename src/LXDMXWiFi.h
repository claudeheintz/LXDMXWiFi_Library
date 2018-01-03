/* LXDMXWiFi.h
   Copyright 2015-2016 by Claude Heintz Design All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of LXDMXWiFi_library nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------------------
*/
#ifndef LXDMXWIFI_H
#define LXDMXWIFI_H

#include <Udp.h>
#include <Arduino.h>
#include <inttypes.h>

//beginPacketMulticast is supported by WiFiUDP in ESP8266WiFi, but not WiFiUDP in WiFi101
//If not using the latest IDE, comment out lines 40 and 41 to use this library with WiFi101, including MKR1000
#if defined(ARDUINO_ARCH_ESP8266)
	#include <WiFiUdp.h>
	#define _UDP_SUPPORTS_BEGINMULTICASTPACKET 1
#endif

#define DMX_UNIVERSE_SIZE 512

#define RESULT_NONE 0
#define RESULT_DMX_RECEIVED 1
#define RESULT_PACKET_COMPLETE 2

/*!   
*  @class LXDMXWiFi
*  @abstract
*     LXDMXWiFi encapsulates functionality for sending and receiving DMX over WiFi.
*     It is a virtual class with concrete subclasses LXWiFiArtNet and LXWiFiSACN which specifically
*     implement the Artistic Licence Art-Net and PLASA sACN 1.31 protocols.
*     
*     Note:  LXDMXWiFi_library requires WiFiUdp 
*            included with the ESP8266 Arduino package.
*
*            Note:  For sending packets larger than 512 bytes, ESP8266 WiFi Library v2.1
*            is required.
*/

class LXDMXWiFi {

  public:
/*!
* @brief UDP port used by protocol
*/
   virtual uint16_t dmxPort      ( void );

/*!
* @brief universe for sending and receiving dmx
* @discussion First universe is zero for Art-Net and one for sACN E1.31.
* Art-Net defines a 15 bit Port-Address net(7)-subnet(4)-universe(4).
* sACN is a full 16 bit but limited to the range 1-63999
* @return universe 0/1-255
*/
   virtual uint16_t universe      ( void );
/*!
* @brief set universe for sending and receiving
* @discussion First universe is zero for Art-Net and one for sACN E1.31.
* Additionally, for Art-Net high nibble is subnet, low nibble is universe.
* For example, setUniverse(0x12) with Art-Net is subnet 1, universe 2,
* the same as setSubnetUniverse(1,2).
* With sACN, setUniverse(0x12) is universe 18.
* @param u universe 0/1-255
*/
   virtual void    setUniverse   ( uint16_t u );
 
 /*!
 * @brief number of slots (aka addresses or channels)
 * @discussion Should be minimum of ~24 depending on actual output speed.  Max of 512.
 * @return number of slots/addresses/channels
 */  
   virtual int  numberOfSlots    ( void );
 /*!
 * @brief set number of slots (aka addresses or channels)
 * @discussion Should be minimum of ~24 depending on actual output speed.  Max of 512.
 * @param n 1 to 512
 */  
   virtual void setNumberOfSlots ( int n );
 /*!
 * @brief get level data from slot/address/channel
 * @param slot 1 to 512
 * @return level for slot (0-255)
 */  
   virtual uint8_t  getSlot      ( int slot );
 /*!
 * @brief set level data (0-255) for slot/address/channel
 * @param slot 1 to 512
 * @param level 0 to 255
 */  
   virtual void     setSlot      ( int slot, uint8_t level );
 /*!
 * @brief direct pointer to dmx buffer uint8_t[]
 * @return uint8_t* to dmx data buffer
 */  
   virtual uint8_t* dmxData      ( void );
   
 /*!
 * @brief direct pointer to packet buffer uint8_t[]
 * @return uint8_t* to packet buffer
 */ 
   virtual uint8_t* packetBuffer      ( void );

/*!
 * @brief size of last packet received with readDMXPacket
 * @return uint16_t last packet size
 */ 
   virtual uint16_t packetSize      ( void );

 /*!
 * @brief read UDP packet
 * @discussion Use this method when the packet buffer is owned by a single object.
 * @param wUDP pointer to UDP object
 * @return 1 if packet contains dmx
 */   
   virtual uint8_t readDMXPacket ( UDP* pUDP );
   
 /*!
 * @brief read contents of packet from _packet_buffer
 * @discussion _packet_buffer should already contain packet payload when this is called.
 *.            Use this method when the packet buffer is shared with other instances.
 * @param wUDP pointer to UDP object
 * @param packetSize size of received packet
 * @return 1 if packet contains dmx
 */      
   virtual uint8_t readDMXPacketContents ( UDP* wUDP, uint16_t packetSize );
   
 /*!
 * @brief send packet for dmx output from network
 * @param wUDP UDP* object to be used for sending UDP packet
 * @param to_ip target address
 * @param interfaceAddr != 0 for multicast
 */  
   virtual void    sendDMX       ( UDP* wUDP, IPAddress to_ip, IPAddress interfaceAddr );
};

#endif // ifndef LXDMXWIFI_H
