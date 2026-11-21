#ifndef BT2_SDP_H
#define BT2_SDP_H

#include "bt2_l2cap.h"

static inline uint16_t
BT_ReadBE16( const uint8_t *data )
{
	return ( ( (uint16_t) data[0] << 8 ) | data[1] );
}

enum BTHCIResult BT2_ParseServiceRecords( struct BTServiceQuery *query, const uint8_t *data, uint32_t length );

#endif
