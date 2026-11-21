
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_obex.h"

// --

int
BT2_HasOBEXReply( const struct BTOBEXPush *push )
{
	return ( ( push->ReplySize ) && ( push->ReplyUsed == push->ReplySize ) );
}

// --

enum BTHCIResult
BT2_ReceiveOBEX( struct BTRFCOMMSession *session, const uint8_t *data, uint32_t length, struct BTHCIError *error )
{
	struct BTOBEXPush *push;

	uint32_t byte;

	push = session->Push;

	if ( length > sizeof( push->Reply ) - push->ReplyUsed )
	{
		return ( BT_HCI_INVALID_REPLY );
	}

	for( byte = 0; byte < length; byte++ )
	{
		push->Reply[push->ReplyUsed + byte] = data[byte];
	}

	push->ReplyUsed += length;

	if ( push->ReplyUsed >= BT_OBEX_HEADER )
	{
		push->ReplySize = BT_OBEXReadBE16( & push->Reply[1] );

		if ( ( push->ReplySize < push->ReplyUsed ) || ( push->ReplySize > sizeof( push->Reply ) ) )
		{
			return ( BT_HCI_INVALID_REPLY );
		}
	}

	return ( BT2_GrantRFCOMMCredit( session, error ) );
}

// --

enum BTHCIResult
BT2_OBEXRequest( struct BTRFCOMMSession *session, uint8_t *packet, uint32_t length,
	uint8_t expected, struct BTHCIError *error )
{
	struct BTOBEXPush *push;
	enum BTHCIResult result;

	uint32_t sent;

	push = session->Push;
	push->ReplySize = 0;
	push->ReplyUsed = 0;
	push->AwaitingReply = 1;
	sent = 0;
	packet[1] = length >> 8;
	packet[2] = length & 0xff;

	// A server can wait for the user to accept the object.
	session->Link->ExtraReadTimeouts = packet[0] == 0x81 ? 0 : 34;

	result = BT2_WriteSerialData( session, packet, length, & sent, error );

	if ( result == BT_HCI_OK )
	{
		result = BT2_WaitRFCOMM( session, BT_RFCOMM_WAIT_OBEX, 0, error );
	}

	session->Link->ExtraReadTimeouts = 0;
	push->AwaitingReply = 0;

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	push->Request->Response = push->Reply[0];

	return ( push->Reply[0] == expected ? BT_HCI_OK : BT_OBEX_REJECTED );
}

// --
