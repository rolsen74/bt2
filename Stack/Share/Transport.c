
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_share.h"

// --

#define SHARE_SDP_CID	 0x0040
#define SHARE_RFCOMM_CID 0x0041

struct ShareChannel
{
	uint16_t Local;
	uint16_t Remote;
	uint16_t MTU;
	uint8_t	 ID;
	uint8_t	 Configured;
};

// --

static enum BTHCIResult
ChannelSignal ( struct BTL2CAPLink * link, struct ShareChannel * channels, const struct BTL2CAPSignal * signal,
				struct BTHCIError * error )
{
	struct ShareChannel * channel;
	enum BTHCIResult	  result;

	uint8_t	 response[8];
	uint16_t value;
	U32		 index;

	if ( ! signal->ID )
	{
		return ( BT_HCI_OK );
	}

	if ( ( signal->Code == 2 ) && ( signal->Length == 4 ) )
	{
		value = BT_ReadLE16 ( signal->Data );

		if ( ( value != 1 ) && ( value != 3 ) )
		{
			return ( BT2_ServiceSignal ( link, signal, error ) );
		}

		channel = &channels[value >> 1];
		value	= BT_ReadLE16 ( signal->Data + 2 );

		if ( ( value < 0x40 ) || ( channel->Remote ) || ( channels[( channel == channels ) ? 1 : 0].Remote == value ) )
		{
			return ( BT_L2CAP_REJECTED );
		}

		channel->Remote		= value;
		channel->Configured = 0;
		channel->MTU		= BT_L2CAP_MTU;
		response[0]			= channel->Local;
		response[1]			= channel->Local >> 8;
		response[2]			= value;
		response[3]			= value >> 8;
		response[4]			= 0;
		response[5]			= 0;
		response[6]			= 0;
		response[7]			= 0;

		result = BT2_WriteSignal ( link, 3, signal->ID, response, sizeof ( response ), error );

		if ( result != BT_HCI_OK )
		{
			return ( result );
		}

		response[0] = value;
		response[1] = value >> 8;
		response[2] = 0;
		response[3] = 0;

		return ( BT2_WriteSignal ( link, 4, channel->ID, response, 4, error ) );
	}

	for ( index = 0; index < 2; index++ )
	{
		channel = &channels[index];

		if ( ( ! channel->Remote ) || ( signal->Length < 4 ) || ( BT_ReadLE16 ( signal->Data ) != channel->Local ) )
		{
			continue;
		}

		switch ( signal->Code )
		{
			case 4:
				link->PeerMTU = channel->MTU;

				result = BT2_ConfigurePeer ( link, signal, channel->Remote, error );

				if ( result == BT_HCI_OK )
				{
					channel->Configured |= 1;
					channel->MTU = link->PeerMTU;
				}

				return ( result );

			case 5:
				if ( ( signal->ID == channel->ID ) && ( signal->Length >= 6 ) && ( ! BT_ReadLE16 ( signal->Data + 2 ) )
					 && ( ! BT_ReadLE16 ( signal->Data + 4 ) ) )
				{
					channel->Configured |= 2;

					return ( BT_HCI_OK );
				}
				break;

			case 6:
				if ( ( signal->Length == 4 ) && ( BT_ReadLE16 ( signal->Data + 2 ) == channel->Remote ) )
				{
					channel->Remote		= 0;
					channel->Configured = 0;

					return ( BT2_WriteSignal ( link, 7, signal->ID, signal->Data, 4, error ) );
				}
				break;
		}
	}

	return ( BT2_ServiceSignal ( link, signal, error ) );
}

// --

enum BTHCIResult
BT2_ShareTransport ( struct BTShareSession * session, struct BTL2CAPLink * link, struct BTHCIError * error )
{
	struct ShareChannel	 channels[2] = { { SHARE_SDP_CID, 0, BT_L2CAP_MTU, 1, 0 }, { SHARE_RFCOMM_CID, 0, BT_L2CAP_MTU, 2, 0 } };
	struct BTL2CAPPacket packet;
	struct BTL2CAPSignal signal;
	enum BTHCIResult	 result;

	const uint8_t * cursor;
	U32				remaining;
	U32				reads;
	U32				idle;

	idle					  = 0;
	session->RFCOMM.Link	  = link;
	session->RFCOMM.Responder = TRUE;

	while ( ! BT2_LinkCancelled ( link ) )
	{
		reads = 128;

		result = BT2_ReadL2CAP ( link, &packet, &reads, error );

		if ( result != BT_HCI_OK )
		{
			if ( ( result == BT_ACL_READ_ERROR ) && ( error->TransportError == BT_HCI_TRANSPORT_TIMEOUT ) && ( ++idle < 7 )
				 && ( BT2_ShareAccess ( session, NULL ) ) && ( BT2_ShareEncrypted ( link, error ) ) )
			{
				continue;
			}

			return ( result );
		}

		idle = 0;

		cursor = packet.Data + BT_L2CAP_HEADER;

		if ( packet.CID == BT_L2CAP_SIGNAL_CID )
		{
			remaining = packet.Length;

			while ( BT2_DecodeSignal ( &cursor, &remaining, &signal ) )
			{
				result = ChannelSignal ( link, channels, &signal, error );

				if ( result != BT_HCI_OK )
				{
					return ( result );
				}
			}

			if ( ( session->RFCOMM.RemoteCID ) && ( ! channels[1].Remote ) )
			{
				return ( BT_HCI_OK );
			}
		}
		else if ( ( packet.CID == SHARE_SDP_CID ) && ( channels[0].Configured == 3 ) )
		{
			link->PeerMTU = channels[0].MTU;

			result = BT2_ShareSDP ( link, channels[0].Remote, cursor, packet.Length, error );
		}
		else if ( ( packet.CID == SHARE_RFCOMM_CID ) && ( channels[1].Configured == 3 ) )
		{
			session->RFCOMM.RemoteCID = channels[1].Remote;
			link->PeerMTU			  = channels[1].MTU;

			result = BT2_ShareRFCOMM ( session, &packet, error );
		}

		if ( result != BT_HCI_OK )
		{
			return ( result );
		}
	}

	return ( BT_RFCOMM_CANCELLED );
}

// --
