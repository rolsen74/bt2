
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_obex.h"

// --

#define OBEX_CONNECT	   0x80
#define OBEX_DISCONNECT	   0x81
#define OBEX_PUT		   0x02
#define OBEX_FINAL		   0x80
#define OBEX_SUCCESS	   0xa0
#define OBEX_CONTINUE	   0x90
#define OBEX_NAME		   0x01
#define OBEX_LENGTH		   0xc3
#define OBEX_BODY		   0x48
#define OBEX_END_BODY	   0x49
#define OBEX_CONNECTION_ID 0xcb
#define OBEX_MIN_PACKET	   255

// --

static enum BTHCIResult
ConnectOBEX ( struct BTRFCOMMSession * session, struct BTHCIError * error )
{
	struct BTOBEXPush * push;
	enum BTHCIResult	result;

	uint8_t			packet[7];
	const uint8_t * header;
	uint32_t		left;
	uint32_t		length;
	uint32_t		byte;

	push				  = session->Push;
	push->HasConnectionID = 0;
	packet[0]			  = OBEX_CONNECT;
	packet[3]			  = 0x10;
	packet[4]			  = 0;
	packet[5]			  = BT_OBEX_MAX_PACKET >> 8;
	packet[6]			  = BT_OBEX_MAX_PACKET & 0xff;

	result = BT2_OBEXRequest ( session, packet, sizeof ( packet ), OBEX_SUCCESS, error );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	if ( push->ReplySize < sizeof ( packet ) )
	{
		return ( BT_HCI_INVALID_REPLY );
	}

	push->PacketLimit = BT_OBEXReadBE16 ( &push->Reply[5] );

	if ( push->PacketLimit < OBEX_MIN_PACKET )
	{
		return ( BT_HCI_INVALID_REPLY );
	}

	if ( push->PacketLimit > BT_OBEX_MAX_PACKET )
	{
		push->PacketLimit = BT_OBEX_MAX_PACKET;
	}

	header = &push->Reply[7];
	left   = push->ReplySize - 7;

	while ( left )
	{
		if ( ( header[0] & 0xc0 ) < 0x80 )
		{
			if ( left < 3 )
			{
				return ( BT_HCI_INVALID_REPLY );
			}

			length = BT_OBEXReadBE16 ( header + 1 );

			if ( length < 3 )
			{
				return ( BT_HCI_INVALID_REPLY );
			}
		}
		else
		{
			length = header[0] & 0x40 ? 5 : 2;
		}

		if ( length > left )
		{
			return ( BT_HCI_INVALID_REPLY );
		}

		if ( header[0] == OBEX_CONNECTION_ID )
		{
			for ( byte = 0; byte < sizeof ( push->ConnectionID ); byte++ )
			{
				push->ConnectionID[byte] = header[1 + byte];
			}

			push->HasConnectionID = 1;
		}

		header += length;
		left -= length;
	}

	return ( BT_HCI_OK );
}

// --

static uint32_t
ObjectHeader ( const struct BTOBEXPush * push, uint8_t * packet, uint8_t command )
{
	uint32_t byte;

	packet[0] = command;

	if ( push->HasConnectionID )
	{
		packet[3] = OBEX_CONNECTION_ID;

		for ( byte = 0; byte < sizeof ( push->ConnectionID ); byte++ )
		{
			packet[4 + byte] = push->ConnectionID[byte];
		}

		return ( 8 );
	}

	return ( BT_OBEX_HEADER );
}

// --

static uint32_t
WriteNameAndLength ( const struct BTOBEXPush * push, uint8_t * packet, uint32_t limit )
{
	uint32_t byte;
	uint32_t used;

	used = 3;

	for ( byte = 0; push->Name[byte]; byte++ )
	{
		// Leave room for the terminating character and the Length header.
		if ( used + 9 > limit )
		{
			return ( 0 );
		}

		packet[used++] = 0;
		packet[used++] = (uint8_t)push->Name[byte];
	}

	packet[used++] = 0;
	packet[used++] = 0;
	packet[0]	   = OBEX_NAME;
	packet[1]	   = used >> 8;
	packet[2]	   = used & 0xff;
	packet[used++] = OBEX_LENGTH;
	packet[used++] = push->Size >> 24;
	packet[used++] = push->Size >> 16;
	packet[used++] = push->Size >> 8;
	packet[used++] = push->Size & 0xff;

	return ( used );
}

// --

static enum BTHCIResult
PutObject ( struct BTRFCOMMSession * session, struct BTHCIError * error )
{
	struct BTOBEXPush * push;
	enum BTHCIResult	result;

	uint8_t	 packet[BT_OBEX_MAX_PACKET];
	uint32_t used;
	uint32_t left;
	uint32_t metadata;
	uint32_t chunk;
	uint32_t have;
	int32_t	 actual;
	int		 first;

	push  = session->Push;
	first = 1;

	do
	{
		used = ObjectHeader ( push, packet, OBEX_PUT );

		if ( first )
		{
			metadata = WriteNameAndLength ( push, packet + used, push->PacketLimit - used - BT_OBEX_HEADER );

			if ( ! metadata )
			{
				return ( BT_OBEX_NAME_TOO_LONG );
			}

			used += metadata;
			first = 0;
		}

		left  = push->Size - push->Request->Sent;
		chunk = push->PacketLimit - used - BT_OBEX_HEADER;

		if ( chunk > left )
		{
			chunk = left;
		}

		have = 0;

		while ( have < chunk )
		{
			if ( BT2_LinkCancelled ( session->Link ) )
			{
				return ( BT_RFCOMM_CANCELLED );
			}

			actual = push->Read ( push->Context, packet + used + BT_OBEX_HEADER + have, chunk - have );

			if ( ( actual <= 0 ) || ( (uint32_t)actual > chunk - have ) )
			{
				return ( BT_RFCOMM_FILE_ERROR );
			}

			have += actual;
		}

		packet[used]	 = chunk == left ? OBEX_END_BODY : OBEX_BODY;
		packet[used + 1] = ( chunk + BT_OBEX_HEADER ) >> 8;
		packet[used + 2] = ( chunk + BT_OBEX_HEADER ) & 0xff;

		if ( chunk == left )
		{
			packet[0] |= OBEX_FINAL;
		}

		result = BT2_OBEXRequest ( session, packet, used + chunk + BT_OBEX_HEADER, chunk == left ? OBEX_SUCCESS : OBEX_CONTINUE,
								   error );

		if ( result != BT_HCI_OK )
		{
			return ( result );
		}

		push->Request->Sent += chunk;
	} while ( push->Request->Sent < push->Size );

	push->Accepted = 1;

	return ( BT_HCI_OK );
}

// --

enum BTHCIResult
BT2_RunObjectPush ( struct BTRFCOMMSession * session, struct BTHCIError * error )
{
	enum BTHCIResult result;

	uint8_t	 packet[8];
	uint32_t length;

	if ( ! session->CreditMode )
	{
		return ( BT_RFCOMM_NEEDS_CREDITS );
	}

	result = ConnectOBEX ( session, error );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	result = PutObject ( session, error );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	length = ObjectHeader ( session->Push, packet, OBEX_DISCONNECT );

	return ( BT2_OBEXRequest ( session, packet, length, OBEX_SUCCESS, error ) );
}

// --
