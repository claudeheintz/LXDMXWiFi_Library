/* LXWiFiArtNet.h
   Copyright 2015 by Claude Heintz Design
   see LXDMXWiFi.h for LICENSE

	Art-Net(TM) Designed by and Copyright Artistic Licence (UK) Ltd
*/

#ifndef LXWIFIARTNET_H
#define LXWIFIARTNET_H

#include <Arduino.h>
#include <WiFiUdp.h>
#include <inttypes.h>
#include "LXDMXWiFi.h"

#define ARTNET_PORT 0x1936
#define ARTNET_BUFFER_MAX 530
#define ARTNET_REPLY_SIZE 239
#define ARTNET_ADDRESS_OFFSET 17

#define ARTNET_ART_POLL 0x2000
#define ARTNET_ART_POLL_REPLY 0x2100
#define ARTNET_ART_DMX 0x5000
#define ARTNET_ART_ADDRESS 0x6000
#define ARTNET_NOP 0

/*!
*  @class LXWiFiArtNet
*  @abstract 
*     LXWiFiArtNet partially implements the Art-Net Ethernet Communication Standard.
*  
*  	LXWiFiArtNet is primarily a node implementation.  It supports output of a single universe
*     of DMX data from the network.  It does not support merge and will only accept
*     packets from the first IP address from which it receives an ArtDMX packet.
*     This can be reset by sending an ArtAddress cancel merge command.
*     
*     When reading packets, LXWiFiArtNet will automatically respond to ArtPoll packets.
*     Depending on the constructor used, it will either broadcast the reply or will
*     reply directly to the sender of the poll.
*  
*     http://www.artisticlicence.com
*/
class LXWiFiArtNet : public LXDMXWiFi {

  public:
/*!
* @brief constructor with address used for ArtPollReply
* @param address sent in ArtPollReply
*/   
   LXWiFiArtNet  ( IPAddress address );
/*!
* @brief constructor creates broadcast address for Poll Reply
* @param address sent in ArtPollReply
* @param subnet_mask used to set broadcast address
*/  
	LXWiFiArtNet  ( IPAddress address, IPAddress subnet_mask );
	
/*!
* @brief destructor for LXWiFiArtNet (does nothing at present)
*/ 
   ~LXWiFiArtNet ( void );

/*!
* @brief UDP port used by protocol
*/   
   uint16_t dmxPort ( void ) { return ARTNET_PORT; }

/*!
* @brief universe for sending and receiving dmx
* @discussion First universe is zero for Art-Net.  High nibble is subnet, low nibble is universe.
* @return universe 0-255
*/   
   uint8_t universe           ( void );
/*!
* @brief set universe for sending and receiving
* @discussion First universe is zero for Art-Net.  High nibble is subnet, low nibble is universe.
* @param u universe 0-255
*/
   void    setUniverse        ( uint8_t u );
/*!
* @brief set subnet/universe for sending and receiving
* @discussion First universe is zero for Art-Net.  Sets separate nibbles: high/subnet, low/universe.
* @param s subnet 0-16
* @param u universe 0-16
*/
   void    setSubnetUniverse  ( uint8_t s, uint8_t u );
/*!
* @brief set universe for sending and receiving
* @discussion First universe is zero for Art-Net.  High nibble is subnet, low nibble is universe.
* 0x7f is no change, otherwise if high bit is set, low nibble becomes universe (subnet remains the same)
* @param u universe 0-16 + flag 0x80
*/
   void    setUniverseAddress ( uint8_t u );
/*!
* @brief set subnet for sending and receiving
* @discussion First universe is zero for Art-Net.  High nibble is subnet, low nibble is universe.
* 0x7f is no change, otherwise if high bit is set, low nibble becomes subnet (universe remains the same)
* @param s subnet 0-16 + flag 0x80
*/
   void    setSubnetAddress   ( uint8_t s );

 /*!
 * @brief number of slots (aka addresses or channels)
 * @discussion Should be minimum of ~24 depending on actual output speed.  Max of 512.
 * @return number of slots/addresses/channels
 */     
   int  numberOfSlots    ( void );
 /*!
 * @brief set number of slots (aka addresses or channels)
 * @discussion Should be minimum of ~24 depending on actual output speed.  Max of 512.
 * @param n number of slots 1 to 512
 */  
   void setNumberOfSlots ( int n );
 /*!
 * @brief get merged level data from slot/address/channel
 * @param slot 1 to 512
 * @return level for slot (0-255)
 */  
   uint8_t  getSlot      ( int slot );
 /*!
 * @brief set level data (0-255) for slot/address/channel
 * @param slot 1 to 512
 * @param level level 0 to 255
 */  
   void     setSlot      ( int slot, uint8_t level );
 /*!
 * @brief direct pointer to dmx portion of packet buffer uint8_t[]
 * @return uint8_t* to dmx data portion of packet buffer
 */ 
   uint8_t* dmxData      ( void );

 /*!
 * @brief read UDP packet
 * @param wUDP WiFiUDP object to be used for getting UDP packet
 * @return 1 if packet contains dmx
 */    
   uint8_t  readDMXPacket       ( WiFiUDP wUDP );
 /*!
 * @brief process packet, reading it into _packet_buffer
 * @param wUDP WiFiUDP (used for Poll Reply if applicable)
 * @return Art-Net opcode of packet
 */
   uint16_t readArtNetPacket    ( WiFiUDP wUDP );
 /*!
 * @brief send Art-Net ArtDMX packet for dmx output from network
 * @param wUDP WiFiUDP object to be used for sending UDP packet
 * @param to_ip target address
 * @param interfaceAddr multicast unused for Art-Net
 */    
   void     sendDMX             ( WiFiUDP wUDP, IPAddress to_ip, IPAddress interfaceAddr );
 /*!
 * @brief send ArtPoll Reply packet for dmx output from network
 * @discussion If broadcast address is defined by passing subnet to constructor, reply is broadcast
 *             Otherwise, reply is unicast to remoteIP belonging to the sender of the poll
 * @param wUDP WiFiUDP object to be used for sending UDP packet
 */  
   void     send_art_poll_reply ( WiFiUDP wUDP );
   
  private:
/*!
* @brief buffer that holds contents of incoming or outgoing packet
* @discussion DMX data is written directly into packet buffer when sending
*             When receiving, data is extracted into one of two buffers
*             depending on the source IP of the sender
*/
  	uint8_t   _packet_buffer[ARTNET_BUFFER_MAX];

/*!
* @brief buffers that hold DMX data from source a, source b and HTP composite
* @discussion data is read into _dmx_buffer_a or _dmx_buffer_b depending on the
*             IP address of the sender, at the same time it is merged into _dmx_buffer_c.
*/
  	uint8_t   _dmx_buffer_a[DMX_UNIVERSE_SIZE];
  	uint8_t   _dmx_buffer_b[DMX_UNIVERSE_SIZE];
  	uint8_t   _dmx_buffer_c[DMX_UNIVERSE_SIZE];
  	
/// number of slots/address/channels
  	int       _dmx_slots;
  	int       _dmx_slots_a;
  	int       _dmx_slots_b;
/// high nibble subnet, low nibble universe
  	uint8_t   _universe;
/// sequence number for sending ArtDMX packets
  	uint8_t   _sequence;

/// address included in poll replies 	
  	IPAddress _my_address;
/// if subnet is supplied in constructor, holds address to broadcast poll replies
  	IPAddress _broadcast_address;
/// first sender of an ArtDMX packet (subsequent senders ignored until cancelMerge)
  	IPAddress _dmx_sender_a;
/// second sender of an ArtDMX packet (subsequent senders ignored until cancelMerge)
  	IPAddress _dmx_sender_b;

/*!
* @brief checks packet for "Art-Net" header
* @return opcode if Art-Net packet
*/
  	uint16_t  parse_header        ( void );	
/*!
* @brief utility for parsing ArtAddress packets
* @return opcode in case command changes dmx data
*/
   uint16_t  parse_art_address   ( void );
   
};

#endif // ifndef LXWIFIARTNET_H