/**************************************************************************/
/*!
    @file     LXWiFiSACN.cpp
    @author   Claude Heintz
    @license  BSD (see LXDMXWiFi.h)
    @copyright 2015-2016 by Claude Heintz All Rights Reserved

    LXWiFiSACN partially implements E1.31, Lightweight streaming protocol
    for transport of DMX512 using ACN, via ESP8266 WiFi connection.
    
    sACN E 1.31 is a public standard published by the PLASA technical standards program
    http://tsp.plasa.org/tsp/documents/published_docs.php

    @section  HISTORY

    v1.0 - First release
    v1.1 - adds ability to use external packet buffer
*/
/**************************************************************************/

#include "LXWiFiSACN.h"

LXWiFiSACN::LXWiFiSACN ( void )
{
	initialize(0);
}

LXWiFiSACN::LXWiFiSACN ( uint8_t* buffer )
{
	initialize(buffer);
}

LXWiFiSACN::~LXWiFiSACN ( void )
{
    if ( _owns_buffer ) {		// if we created this buffer, then free the memory
		free(_packet_buffer);
	}
}

void  LXWiFiSACN::initialize  ( uint8_t* b ) {
	if ( b == 0 ) {
		// create buffer
		_packet_buffer = (uint8_t*) malloc(SACN_BUFFER_MAX);
		_owns_buffer = 1;
	} else {
		// external buffer
		_packet_buffer = b;
		_owns_buffer = 0;
	}
	
	//zero buffer including _dmx_data[0] which is start code
    for (int n=0; n<SACN_BUFFER_MAX; n++) {
    	_packet_buffer[n] = 0;
    	if ( n <= DMX_UNIVERSE_SIZE ) {
    	   _dmx_buffer_a[n] = 0;
	   	_dmx_buffer_b[n] = 0;
	   	_dmx_buffer_c[n] = 0;
	   	if ( n < SACN_CID_LENGTH ) {
    		    _dmx_sender_id_a[n] = 0;
    		    _dmx_sender_id_b[n] = 0;
    	   }
    	}
    }
    
    _dmx_slots = 0;
    _dmx_slots_a = 0;
    _dmx_slots_b = 0;
    _universe = 1;                    // NOTE: unlike Art-Net, sACN universes begin at 1
    _sequence = 1;
}

uint8_t  LXWiFiSACN::universe ( void ) {
	return _universe;
}

void LXWiFiSACN::setUniverse ( uint8_t u ) {
	_universe = u;
}

int  LXWiFiSACN::numberOfSlots ( void ) {
	return _dmx_slots;
}

void LXWiFiSACN::setNumberOfSlots ( int n ) {
	_dmx_slots = n;
}

uint8_t LXWiFiSACN::getSlot ( int slot ) {
	return _dmx_buffer_c[slot];
}

void LXWiFiSACN::setSlot ( int slot, uint8_t level ) {
	_packet_buffer[SACN_ADDRESS_OFFSET+slot] = level;
}

uint8_t LXWiFiSACN::startCode ( void ) {
	return _packet_buffer[SACN_ADDRESS_OFFSET];
}

void LXWiFiSACN::setStartCode ( uint8_t value ) {
	_packet_buffer[SACN_ADDRESS_OFFSET] = value;
}

uint8_t* LXWiFiSACN::dmxData( void ) {
	return &_packet_buffer[SACN_ADDRESS_OFFSET];
}

uint8_t* LXWiFiSACN::packetBuffer( void ) {
	return &_packet_buffer[0];
}

uint16_t LXWiFiSACN::packetSize( void ) {
	return _packetSize;
}

uint8_t LXWiFiSACN::readDMXPacket ( WiFiUDP wUDP ) {
	_packetSize = 0;
   if ( readSACNPacket(wUDP) > 0 ) {
   	if ( startCode() == 0 ) {
   		return RESULT_DMX_RECEIVED;
   	}
   }	
   return RESULT_NONE;
}

uint8_t LXWiFiSACN::readDMXPacketContents ( WiFiUDP wUDP, uint16_t packetSize ) {
	if ( parse_root_layer(packetSize) > 0 ) {
   	if ( startCode() == 0 ) {
   		return RESULT_DMX_RECEIVED;
   	}
   }	
   return RESULT_NONE;
}

uint16_t LXWiFiSACN::readSACNPacket ( WiFiUDP wUDP ) {
   _dmx_slots = 0;
   uint16_t packetSize = wUDP.parsePacket();
   if ( packetSize ) {
      _packetSize = wUDP.read(_packet_buffer, SACN_BUFFER_MAX);
      _dmx_slots = parse_root_layer(_packetSize);
   }
   return _dmx_slots;
}

void LXWiFiSACN::sendDMX ( WiFiUDP wUDP, IPAddress to_ip, IPAddress interfaceAddr ) {
   for (int n=0; n<126; n++) {
    	_packet_buffer[n] = 0;		// zero outside layers & start code
    }
   _packet_buffer[0] = 0;
   _packet_buffer[1] = 0x10;
   _packet_buffer[4] = 'A';
   _packet_buffer[5] = 'S';
   _packet_buffer[6] = 'C';
   _packet_buffer[7] = '-';
   _packet_buffer[8] = 'E';
   _packet_buffer[9] = '1';
   _packet_buffer[10] = '.';
   _packet_buffer[11] = '1';
   _packet_buffer[12] = '7';
   uint16_t fplusl = _dmx_slots + 110 + 0x7000;
   _packet_buffer[16] = fplusl >> 8;
   _packet_buffer[17] = fplusl & 0xff;
   _packet_buffer[21] = 0x04;
   fplusl = _dmx_slots + 88 + 0x7000;
   _packet_buffer[38] = fplusl >> 8;
   _packet_buffer[39] = fplusl & 0xff;
   _packet_buffer[43] = 0x02;
   _packet_buffer[44] = 'A';
   _packet_buffer[45] = 'r';
   _packet_buffer[46] = 'd';
   _packet_buffer[47] = 'u';
   _packet_buffer[48] = 'i';
   _packet_buffer[49] = 'n';
   _packet_buffer[50] = 'o';
   _packet_buffer[108] = 100;	//priority
   if ( _sequence == 0 ) {
     _sequence = 1;
   } else {
     _sequence++;
   }
   _packet_buffer[111] = _sequence;
   _packet_buffer[113] = _universe >> 8;
   _packet_buffer[114] = _universe & 0xff;
   fplusl = _dmx_slots + 11 + 0x7000;
   _packet_buffer[115] = fplusl >> 8;
   _packet_buffer[116] = fplusl & 0xff;
   _packet_buffer[117] = 0x02;
   _packet_buffer[118] = 0xa1;
   _packet_buffer[122] = 0x01;
   fplusl = _dmx_slots + 1;
   _packet_buffer[123] = fplusl >> 8;
   _packet_buffer[124] = fplusl & 0xFF;
   //assume dmx data has been set
   if ( interfaceAddr != 0 ) {
      wUDP.beginPacketMulticast(to_ip, SACN_PORT, interfaceAddr);
   } else {
      wUDP.beginPacket(to_ip, SACN_PORT);
   }
   wUDP.write(_packet_buffer, _dmx_slots + 126);
   wUDP.endPacket();
}

uint16_t LXWiFiSACN::parse_root_layer( uint16_t size ) {
  if  ( _packet_buffer[1] == 0x10 ) {									//preamble size
    if ( strcmp((const char*)&_packet_buffer[4], "ASC-E1.17") == 0 ) {
      uint16_t tsize = size - 16;
      if ( checkFlagsAndLength(&_packet_buffer[16], tsize) ) { // root pdu length
        if ( _packet_buffer[21] == 0x04 ) {							// vector RLP is 1.31 data
          return parse_framing_layer( tsize );
        }
      }
    }       // ACN packet identifier
  }			// preamble size
  return 0;
}

uint16_t LXWiFiSACN::parse_framing_layer( uint16_t size ) {
   uint16_t tsize = size - 22;
   if ( checkFlagsAndLength(&_packet_buffer[38], tsize) ) {     // framing pdu length
     if ( _packet_buffer[43] == 0x02 ) {                        // vector dmp is 1.31
        if ( _packet_buffer[114] == _universe ) { // implementation has 255 universe limit
          return parse_dmp_layer( tsize );    
        }
     }
   }
   return 0;
}

uint8_t compareCID(uint8_t* cid_a, uint8_t* cid_b) {
	for(int k=0; k<SACN_CID_LENGTH; k++) {
	   if ( cid_a[k] != cid_b[k] ) {
         return 0;
      }
	}
	return 1;
}

uint16_t LXWiFiSACN::parse_dmp_layer( uint16_t size ) {
  uint16_t tsize = size - 77;
  if ( checkFlagsAndLength(&_packet_buffer[115], tsize) ) {  // dmp pdu length
    if ( _packet_buffer[117] == 0x02 ) {                     // Set Property
      if ( _packet_buffer[118] == 0xa1 ) {                   // address and data format
        uint16_t dsize = _packet_buffer[124] + (_packet_buffer[123] << 8);
        if ( dsize != (tsize - 10) ) {
           return 0;
        }
        
        if ( _dmx_sender_id_a[0] == 0  ) {			
          for(int k=0; k<SACN_CID_LENGTH; k++) {		 // if _dmx_sender_id is not set
            _dmx_sender_id_a[k] = _packet_buffer[k+22];  // set it to id of this packet
          }
        }
        if ( compareCID(&_dmx_sender_id_a[0], &_packet_buffer[22]) ) {
           _dmx_slots_a = dsize;
           int di;
			  for (di=0; di<_dmx_slots_a; di++) {
				 _dmx_buffer_a[di] = _packet_buffer[SACN_ADDRESS_OFFSET+di];
				 if ( _dmx_buffer_a[di] > _dmx_buffer_b[di] ) {
					_dmx_buffer_c[di] = _dmx_buffer_a[di];
				 } else {
					_dmx_buffer_c[di] = _dmx_buffer_b[di];
				 }
			  }
           int slots = _dmx_slots_a -1;					//remove extra 1 for start code
           if ( _dmx_slots_b > _dmx_slots_a ) {
              slots = _dmx_slots_b;
           }
           return slots;
        } else { //CID match
           if ( _dmx_sender_id_b[0] == 0  ) {
              for(int k=0; k<SACN_CID_LENGTH; k++) {		 // if _dmx_sender_id is not set
                 _dmx_sender_id_b[k] = _packet_buffer[k+22];  // set it to id of this packet
               }
           }
           if ( compareCID(&_dmx_sender_id_b[0], &_packet_buffer[22]) ) {
              _dmx_slots_b = dsize;
              int di;
				  for (di=0; di<_dmx_slots_b; di++) {
					 _dmx_buffer_b[di] = _packet_buffer[SACN_ADDRESS_OFFSET+di];
					 if ( _dmx_buffer_a[di] > _dmx_buffer_b[di] ) {
						_dmx_buffer_c[di] = _dmx_buffer_a[di];
					 } else {
						_dmx_buffer_c[di] = _dmx_buffer_b[di];
					 }
				  }
				  int slots = _dmx_slots_b - 1;					//remove extra 1 for start code
				  if ( _dmx_slots_a > _dmx_slots_b ) {
					  slots = _dmx_slots_a;
				  }
              return slots;
           } //CID match
        }
        
      }
    }
  }
  return 0;
}

//  utility for checking 2 byte:  flags (high nibble == 0x7) && 12 bit length

uint8_t LXWiFiSACN::checkFlagsAndLength( uint8_t* flb, uint16_t size ) {
	if ( ( flb[0] & 0xF0 ) == 0x70 ) {
	   uint16_t pdu_length = flb[1];
		pdu_length += ((flb[0] & 0x0f) << 8);
		if ( ( pdu_length != 0 ) && ( size >= pdu_length ) ) {
		   return 1;
		}
	}
	return 0;
}
