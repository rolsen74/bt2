#ifndef BT2_SDP_H
#define BT2_SDP_H

#include "bt2_l2cap.h"

static inline uint16_t
BT_ReadBE16( const uint8_t *data )
{
	return ( ( (uint16_t) data[0] << 8 ) | data[1] );
}

enum BTHCIResult BT2_ParseServiceRecords( struct BTServiceQuery *query, const uint8_t *data, uint32_t length );

enum BTHCIResult BT2_SearchServices( const struct BTACLTransport *transport, struct BTServiceQuery *query,
	uint16_t uuid, const uint32_t *cancel, struct BTHCIError *error );
enum BTHCIResult BT2_FindObjectPushChannel( const struct BTACLTransport *transport,
	const uint8_t *address, uint8_t *channel, const uint32_t *cancel, struct BTHCIError *error );

#endif
