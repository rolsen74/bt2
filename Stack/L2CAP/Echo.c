
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <stddef.h>
#include "bt2_l2cap.h"

// --

#define L2CAP_ECHO_REQUEST 0x08
#define L2CAP_ECHO_RESPONSE 0x09
#define L2CAP_ECHO_IDENTIFIER 1

// --

static enum BTHCIResult
ReadEchoResponse( struct BTL2CAPLink *link, struct BTHCIError *error )
{
	struct BTL2CAPPacket pdu;
	struct BTL2CAPSignal signal;
	enum BTHCIResult result;

	const uint8_t *cursor;
	uint32_t remaining;
	uint32_t reads;

	reads = 256;

	while( reads )
	{
		result = BT2_ReadL2CAP( link, & pdu, & reads, error );

		if ( result != BT_HCI_OK )
		{
			return ( result );
		}

		if ( pdu.CID != BT_L2CAP_SIGNAL_CID )
		{
			continue;
		}

		cursor = & pdu.Data[BT_L2CAP_HEADER];
		remaining = pdu.Length;

		while( BT2_DecodeSignal( & cursor, & remaining, & signal ) )
		{
			if ( signal.ID != L2CAP_ECHO_IDENTIFIER )
			{
				continue;
			}

			if ( signal.Code == L2CAP_ECHO_RESPONSE )
			{
				return ( BT_HCI_OK );
			}

			if ( ( signal.Code == BT_L2CAP_SIGNAL_REJECT ) && ( signal.Length >= 2 ) )
			{
				error->Status = signal.Data[0];

				return ( BT_L2CAP_REJECTED );
			}
		}
	}

	return ( BT_HCI_INVALID_REPLY );
}

// --

enum BTHCIResult
BT_L2CAPEcho( const struct BTACLTransport *transport, const uint8_t *address, struct BTHCIError *error )
{
	struct BTL2CAPLink link;
	enum BTHCIResult result;

	result = BT2_OpenDataLink( & link, transport, address, error );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	result = BT2_WriteSignal( & link, L2CAP_ECHO_REQUEST, L2CAP_ECHO_IDENTIFIER, NULL, 0, error );

	if ( result == BT_HCI_OK )
	{
		result = ReadEchoResponse( & link, error );
	}

	return ( BT2_CloseDataLink( & link, result, error ) );
}

// --
