
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_sdp.h"

// --

static int
IsObjectPush ( const struct BTServiceRecord * record )
{
	static const uint8_t uuid[] = { 0, 0, 0x11, 0x05, 0, 0, 0x10, 0, 0x80, 0, 0, 0x80, 0x5f, 0x9b, 0x34, 0xfb };
	uint32_t			 start;
	uint32_t			 byte;

	switch ( record->UUIDLength )
	{
		case 2:
			start = 2;
			break;
		case 4:
		case 16:
			start = 0;
			break;
		default:
			return ( 0 );
	}

	for ( byte = 0; byte < record->UUIDLength; byte++ )
	{
		if ( record->UUID[byte] != uuid[start + byte] )
		{
			return ( 0 );
		}
	}

	return ( 1 );
}

// --

enum BTHCIResult
BT2_FindObjectPushChannel ( const struct BTACLTransport * transport, const uint8_t * address, uint8_t * channel,
							const uint32_t * cancel, struct BTHCIError * error )
{
	struct BTServiceRecord records[8];
	struct BTServiceQuery  query;
	enum BTHCIResult	   result;

	uint32_t index;

	query.Records  = records;
	query.Capacity = sizeof ( records ) / sizeof ( records[0] );

	for ( index = 0; index < sizeof ( query.Address ); index++ )
	{
		query.Address[index] = address[index];
	}

	result = BT2_SearchServices ( transport, &query, 0x1105, cancel, error );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	for ( index = 0; index < query.Count; index++ )
	{
		if ( ( records[index].RFCOMMChannel >= 1 ) && ( records[index].RFCOMMChannel <= 30 )
			 && ( IsObjectPush ( &records[index] ) ) )
		{
			*channel = records[index].RFCOMMChannel;

			return ( BT_HCI_OK );
		}
	}

	return ( BT_OBEX_NO_SERVICE );
}

// --
