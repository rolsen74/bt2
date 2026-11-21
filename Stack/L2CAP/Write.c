
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_l2cap.h"

// --

#define HCI_EVENT_DISCONNECTION 0x05
#define HCI_EVENT_COMPLETED_PACKETS 0x13
#define HCI_ACL_PB_FIRST 0x2000
#define HCI_ACL_PB_CONTINUATION 0x1000
#define HCI_ACL_HEADER 4

// --

static enum BTHCIResult
WaitForCredit( struct BTL2CAPLink *link, struct BTHCIError *error )
{
	enum BTHCIResult result;

	uint8_t event[HCI_EVENT_MAX];
	uint32_t reads;
	uint32_t length;
	uint32_t offset;
	uint32_t credits;

	reads = 64;

	while( reads )
	{
		if ( BT2_LinkCancelled( link ) )
		{
			return ( BT_RFCOMM_CANCELLED );
		}

		result = BT2_ReadEvent( & link->Transport->HCI, event, & length, & reads, error );

		if ( result != BT_HCI_OK )
		{
			if ( result == BT_HCI_READ_ERROR )
			{
				return ( BT_ACL_READ_ERROR );
			}

			return ( result );
		}

		if ( ( event[0] == HCI_EVENT_DISCONNECTION ) && ( length == 6 ) && ( ! event[2] ) )
		{
			if ( BT_ReadLE16( & event[3] ) == link->Handle )
			{
				error->Status = event[5];

				return ( BT_L2CAP_REJECTED );
			}
		}

		if ( ( event[0] != HCI_EVENT_COMPLETED_PACKETS ) || ( length < 3 ) ||
			( length != 3 + 4U * event[2] ) )
		{
			continue;
		}

		for( offset = 3; offset < length; offset += 4 )
		{
			if ( BT_ReadLE16( & event[offset] ) == link->Handle )
			{
				credits = link->Credits + BT_ReadLE16( & event[offset + 2] );
				link->Credits = credits > link->CreditLimit ? link->CreditLimit : credits;
			}
		}

		if ( link->Credits )
		{
			return ( BT_HCI_OK );
		}
	}

	return ( BT_HCI_INVALID_REPLY );
}

// --

enum BTHCIResult
BT2_WriteL2CAP( struct BTL2CAPLink *link, uint16_t cid, const uint8_t *data, uint32_t length, struct BTHCIError *error )
{
	enum BTHCIResult result;

	uint8_t packet[HCI_ACL_HEADER + BT_L2CAP_HEADER + BT_L2CAP_MTU];
	uint8_t basic[BT_L2CAP_HEADER];
	uint16_t header;
	uint32_t offset;
	uint32_t size;
	uint32_t byte;

	if ( length > BT_L2CAP_MTU )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	error->Opcode = 0;
	error->Status = 0;
	basic[0] = length & 0xff;
	basic[1] = length >> 8;
	basic[2] = cid & 0xff;
	basic[3] = cid >> 8;
	offset = 0;

	while( offset < BT_L2CAP_HEADER + length )
	{
		if ( BT2_LinkCancelled( link ) )
		{
			return ( BT_RFCOMM_CANCELLED );
		}

		if ( ! link->Credits )
		{
			result = WaitForCredit( link, error );

			if ( result != BT_HCI_OK )
			{
				return ( result );
			}
		}

		size = BT_L2CAP_HEADER + length - offset;

		if ( size > link->PacketSize )
		{
			size = link->PacketSize;
		}

		header = link->Handle | ( offset ? HCI_ACL_PB_CONTINUATION : HCI_ACL_PB_FIRST );
		packet[0] = header & 0xff;
		packet[1] = header >> 8;
		packet[2] = size & 0xff;
		packet[3] = size >> 8;

		for( byte = 0; byte < size; byte++ )
		{
			packet[HCI_ACL_HEADER + byte] = offset < BT_L2CAP_HEADER ? basic[offset] : data[offset - BT_L2CAP_HEADER];
			offset++;
		}

		error->TransportError = link->Transport->WriteACL( link->Transport->HCI.Context, packet, size + HCI_ACL_HEADER );

		if ( error->TransportError )
		{
			return ( BT_ACL_WRITE_ERROR );
		}

		link->Credits--;
	}

	return ( BT_HCI_OK );
}

// --
