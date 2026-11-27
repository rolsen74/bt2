
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_l2cap.h"

// --

#define HCI_ACL_HEADER_SIZE		4
#define HCI_ACL_PACKET_MAX		( 4096 + HCI_ACL_HEADER_SIZE )
#define HCI_ACL_HANDLE_MASK		0x0fff
#define HCI_ACL_PB_CONTINUATION 0x1000
#define HCI_ACL_PB_MASK			0x3000
#define HCI_ACL_BROADCAST_MASK	0xc000

// --

static enum BTHCIResult
ReadACLFrame ( const struct BTACLTransport * transport, uint8_t * packet, uint32_t * length, uint32_t * reads,
			   struct BTHCIError * error )
{
	const uint8_t * chunk;
	uint32_t		chunkLength;
	uint32_t		total;
	uint32_t		byte;

	*length = 0;
	total	= HCI_ACL_PACKET_MAX;

	while ( *reads )
	{
		( *reads )--;

		error->TransportError = transport->ReadACLChunk ( transport->HCI.Context, &chunk, &chunkLength );

		if ( error->TransportError )
		{
			return ( BT_ACL_READ_ERROR );
		}

		if ( chunkLength > total - *length )
		{
			break;
		}

		for ( byte = 0; byte < chunkLength; byte++ )
		{
			packet[*length + byte] = chunk[byte];
		}

		*length += chunkLength;

		if ( *length >= HCI_ACL_HEADER_SIZE )
		{
			total = HCI_ACL_HEADER_SIZE + BT_ReadLE16 ( &packet[2] );

			if ( ( total > HCI_ACL_PACKET_MAX ) || ( *length > total ) )
			{
				break;
			}

			if ( *length == total )
			{
				return ( BT_HCI_OK );
			}
		}
	}

	return ( BT_HCI_INVALID_REPLY );
}

// --

enum BTHCIResult
BT2_ReadL2CAPWithTimeouts ( struct BTL2CAPLink * link, struct BTL2CAPPacket * pdu, uint32_t * reads, uint32_t retryLimit,
							struct BTHCIError * error )
{
	enum BTHCIResult result;

	uint8_t	 packet[HCI_ACL_PACKET_MAX];
	uint32_t timeouts;
	uint32_t length;
	uint32_t used;
	uint32_t byte;
	uint16_t header;
	uint16_t pduLength;
	int		 assembling;

	timeouts	  = 0;
	used		  = 0;
	assembling	  = 0;
	error->Opcode = 0;
	error->Status = 0;

	while ( *reads )
	{
		if ( BT2_LinkCancelled ( link ) )
		{
			return ( BT_RFCOMM_CANCELLED );
		}

		result = ReadACLFrame ( link->Transport, packet, &length, reads, error );

		if ( result != BT_HCI_OK )
		{
			if ( ( result == BT_ACL_READ_ERROR ) && ( error->TransportError == BT_HCI_TRANSPORT_TIMEOUT ) && ( ! length )
				 && ( ++timeouts < retryLimit ) )
			{
				continue;
			}

			return ( result );
		}

		header = BT_ReadLE16 ( packet );

		if ( ( ( header & HCI_ACL_HANDLE_MASK ) != link->Handle ) || ( header & HCI_ACL_BROADCAST_MASK ) )
		{
			continue;
		}

		if ( ( header & HCI_ACL_PB_MASK ) != HCI_ACL_PB_CONTINUATION )
		{
			used	   = 0;
			assembling = 1;
		}

		length -= HCI_ACL_HEADER_SIZE;

		if ( ( ! assembling ) || ( length > sizeof ( pdu->Data ) - used ) )
		{
			assembling = 0;

			continue;
		}

		for ( byte = 0; byte < length; byte++ )
		{
			pdu->Data[used + byte] = packet[HCI_ACL_HEADER_SIZE + byte];
		}

		used += length;

		if ( used < BT_L2CAP_HEADER )
		{
			continue;
		}

		pduLength = BT_ReadLE16 ( pdu->Data );

		if ( ( pduLength > BT_L2CAP_MTU ) || ( used > BT_L2CAP_HEADER + (uint32_t)pduLength ) )
		{
			assembling = 0;

			continue;
		}

		if ( used == BT_L2CAP_HEADER + (uint32_t)pduLength )
		{
			pdu->Length = pduLength;
			pdu->CID	= BT_ReadLE16 ( &pdu->Data[2] );

			return ( BT_HCI_OK );
		}
	}

	return ( BT_HCI_INVALID_REPLY );
}

// --

enum BTHCIResult
BT2_ReadL2CAP ( struct BTL2CAPLink * link, struct BTL2CAPPacket * pdu, uint32_t * reads, struct BTHCIError * error )
{
	return ( BT2_ReadL2CAPWithTimeouts ( link, pdu, reads, 6 + link->ExtraReadTimeouts, error ) );
}

// --
