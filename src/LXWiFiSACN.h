/* LXWiFiSACN.h
   Copyright 2015 by Claude Heintz Design
   see LXDMXWiFi.h for LICENSE
   
   sACN E 1.31 is a public standard published by the PLASA technical standards program
   http://tsp.plasa.org/tsp/documents/published_docs.php
*/

#ifndef LXWIFISACNDMX_H
#define LXWIFISACNDMX_H

#include <Arduino.h>
#include <inttypes.h>
#include "LXDMXWiFi.h"

#define SACN_PORT 0x15C0
#define SACN_BUFFER_MAX 638
#define SACN_PRIORITY_OFFSET 108
#define SACN_ADDRESS_OFFSET 125
#define SACN_CID_LENGTH 16
#define SLOTS_AND_START_CODE 513

/*!
* @class LXWiFiSACN
* @abstract
*          LXWiFiSACN partially implements E1.31, lightweight streaming protocol for transport of DMX512 using ACN
*          http://tsp.plasa.org/tsp/documents/published_docs.php
*          
*          	LXWiFiSACN is primarily a node implementation.  It supports output of a single universe
*          	of DMX data from the network.  It does not support merge and will only accept
*
*          	packets from the first source from which it receives an E1.31 DMX packet.
*/
class LXWiFiSACN : public LXDMXWiFi {

  public:
/*!
* @brief constructor for LXWiFiSACN
*/  
	LXWiFiSACN  ( void );
/*!
* @brief constructor for LXWiFiSACN with external buffer fro UDP packets
* @param external packet buffer
*/  
	LXWiFiSACN ( uint8_t* buffer );
/*!
* @brief destructor for LXWiFiSACN  (frees packet buffer if allocated with constructor)
*/  
   ~LXWiFiSACN ( void );

/*!
* @brief UDP port used by protocol
*/   
   uint16_t dmxPort ( void ) { return SACN_PORT; }

/*!
* @brief universe for sending and receiving dmx
* @discussion First universe is one for sACN E1.31.  Range is 1-255 in this implementation, 1-32767 in full E1.31.
* @return universe 1-255
*/ 
   uint16_t universe      ( void );
/*!
* @brief set universe for sending and receiving
* @discussion First universe one for sACN E1.31.  Range is 1-255 in this implementation, 1-32767 in full E1.31.
* @param u universe 1-255
*/
   void    setUniverse   ( uint16_t u );

 /*
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
 * @brief get level data from slot/address/channel
 * @param slot 1 to 512
 * @return level for slot (0-255)
 */  
   uint8_t  getSlot      ( int slot );
 /*!
 * @brief set level data (0-255) for slot/address/channel
 * @param slot 1 to 512
 * @param level 0 to 255
 */  
   void     setSlot      ( int slot, uint8_t level );
/*!
* @brief dmx start code (set to zero for standard dmx)
* @return dmx start code
*/
   uint8_t  startCode    ( void );
/*!
* @brief sets dmx start code for outgoing packets
* @param value start code (set to zero for standard dmx)
*/
   void     setStartCode ( uint8_t value );
 /*!
 * @brief direct pointer to dmx buffer uint8_t[]
 * @return uint8_t* to dmx data buffer
 */
   uint8_t* dmxData      ( void );

 /*!
 * @brief direct pointer to packet buffer uint8_t[]
 * @return uint8_t* to packet buffer
 */ 
   uint8_t* packetBuffer      ( void );
   
/*!
 * @brief size of last packet received with readDMXPacket
 * @return uint16_t last packet size
 */ 
   uint16_t packetSize      ( void );

 /*!
 * @brief read UDP packet
 * @param wUDP pointer to UDP object
 * @return 1 if packet contains dmx
 */    
   uint8_t  readDMXPacket  ( UDP* wUDP );
 /*!
 * @brief process packet, reading it into _packet_buffer
 * @param wUDP pointer to UDP object
 * @return number of dmx slots read or 0 if not dmx/invalid
 */
   uint16_t readSACNPacket ( UDP* wUDP );
   
 /*!
 * @brief read contents of packet from _packet_buffer
 * @discussion _packet_buffer should already contain packet payload when this is called
 * @param wUDP pointer to UDP object
 * @param packetSize size of received packet
 * @return 1 if packet contains dmx
 */      
   uint8_t readDMXPacketContents ( UDP* wUDP, uint16_t packetSize );
   
 /*!
 * @brief send sACN E1.31 packet for dmx output from network
 * @param wUDP pointer to UDP object to be used to send packet
 * @param to_ip target address
 * @param interfaceAddr != 0 for multicast
 */  
   void sendDMX ( UDP* wUDP, IPAddress to_ip, IPAddress interfaceAddr );
   
 /*!
 * @brief clear dmx buffers and sender CIDs
 */    
   void clearDMXOutput ( void );

   
  private:
/*!
* @brief buffer that holds contents of incoming or outgoing packet
* @discussion readSACNPacket fills the buffer with the payload of the incoming packet.
*             this is then copied into _dmx_buffer_a or _dmx_buffer_b depending on the sender
*             at the same time it is merged HTP into _dmx_buffer_c
*             DMX data to be sent is written directly to _packet_buffer
*/
  	uint8_t*   _packet_buffer;
/*!
* @brief indicates was created by constructor
*/
	uint8_t   _owns_buffer;
/*!
* @brief size of last packet that was read
*/	uint16_t  _packetSize;
  	
/*!
* @brief buffers that hold DMX data from source a, source b and HTP composite
* @discussion data is read into _dmx_buffer_a or _dmx_buffer_b depending on the
*             IP address of the sender, at the same time it is merged into _dmx_buffer_c.
*/
  	uint8_t   _dmx_buffer_a[DMX_UNIVERSE_SIZE+1];
  	uint8_t   _dmx_buffer_b[DMX_UNIVERSE_SIZE+1];
  	uint8_t   _dmx_buffer_c[DMX_UNIVERSE_SIZE+1];
  	
/// number of slots/address/channels
  	int       _dmx_slots;
  	int       _dmx_slots_a;
  	int       _dmx_slots_b;
  	uint8_t   _priority_a;
  	uint8_t   _priority_b;
  	long      _last_packet_a;
  	long      _last_packet_b;
/// universe 1-255 in this implementation
  	uint16_t  _universe;
/// sequence number for sending sACN DMX packets
  	uint8_t   _sequence;
/// cid of first sender of an E 1.31 DMX packet (subsequent senders ignored)
  	uint8_t _dmx_sender_id_a[SACN_CID_LENGTH];
  	uint8_t _dmx_sender_id_b[SACN_CID_LENGTH];

/*!
* @brief checks the buffer for the sACN header and root layer size
*/  	
  	uint16_t  parse_root_layer    ( uint16_t size );
/*!
* @brief checks the buffer for the sACN header and root layer size
*/  
  	uint16_t  parse_framing_layer ( uint16_t size );	
  	uint16_t  parse_dmp_layer     ( uint16_t size );
  	uint8_t   checkFlagsAndLength ( uint8_t* flb, uint16_t size );
  	
/*!
* @brief initialize data structures
*/
   void  initialize  ( uint8_t* b );
   
 /*!
 * @brief clear "b" dmx buffer and sender CID
 */    
   void clearDMXSourceB ( void );
   
};

#endif // ifndef LXWIFISACNDMX_H