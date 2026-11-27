
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_a2dp.h"

// --

static enum BTHCIResult
HandlePeerCommand ( struct BTAVDTPSession * session, const uint8_t * data, uint32_t length, struct BTHCIError * error )
{
	uint8_t	 reply[12];
	uint32_t size;

	reply[0] = ( data[0] & 0xf0 ) | 2;
	reply[1] = data[1] & 0x3f;
	size	 = 3;

	switch ( reply[1] )
	{
		case 1: // Discover our audio source endpoint.
			if ( length == 2 )
			{
				reply[2] = 4 | ( session->Sink ? 2 : 0 );
				reply[3] = 0;
				size	 = 4;
			}
			else
			{
				reply[0] |= 1;
				reply[2] = 0x11; // Bad length.
			}
			break;

		case 2:	   // Get Capabilities.
		case 0x0c: // Get All Capabilities.
			if ( ( length == 3 ) && ( data[2] == 4 ) )
			{
				reply[2]  = 1;
				reply[3]  = 0;
				reply[4]  = 7;
				reply[5]  = 6;
				reply[6]  = 0;
				reply[7]  = 0;
				reply[8]  = 0x3b; // 44.1/48 kHz, mono, stereo and joint stereo.
				reply[9]  = 0x15; // 16 blocks, eight bands, loudness.
				reply[10] = 2;
				reply[11] = 35;
				size	  = sizeof ( reply );
			}
			else
			{
				reply[0] |= 1;
				reply[2] = ( length == 3 ) ? 0x12 : 0x11;
			}
			break;

		default:
			reply[0] = ( data[0] & 0xf0 ) | 1;
			size	 = 2;
			break;
	}

	return ( BT2_WriteL2CAP ( session->Link, session->SignalCID, reply, size, error ) );
}

// --

enum BTHCIResult
BT2_AVDTPCommand ( struct BTAVDTPSession * session, uint8_t signalID, const uint8_t * data, uint8_t length,
				   struct BTL2CAPPacket * response, struct BTHCIError * error )
{
	struct BTL2CAPSignal signal;
	enum BTHCIResult	 result;

	const uint8_t * cursor;
	uint8_t			command[16];
	uint32_t		reads;
	uint32_t		remaining;
	uint32_t		byte;
	uint8_t			label;

	if ( length > sizeof ( command ) - 2 )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	session->Source->Request->Signal = signalID;
	session->Source->Request->Status = 0;
	label							 = ( session->Transaction++ & 15 ) << 4;
	command[0]						 = label;
	command[1]						 = signalID;

	for ( byte = 0; byte < length; byte++ )
	{
		command[2 + byte] = data[byte];
	}

	result = BT2_WriteL2CAP ( session->Link, session->SignalCID, command, 2 + length, error );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	reads = 256;

	while ( reads )
	{
		result = BT2_ReadL2CAP ( session->Link, response, &reads, error );

		if ( result != BT_HCI_OK )
		{
			return ( result );
		}

		cursor	  = response->Data + BT_L2CAP_HEADER;
		remaining = response->Length;

		if ( response->CID == BT_L2CAP_SIGNAL_CID )
		{
			while ( BT2_DecodeSignal ( &cursor, &remaining, &signal ) )
			{
				result = BT2_ServiceSignal ( session->Link, &signal, error );

				if ( result != BT_HCI_OK )
				{
					return ( result );
				}
			}

			continue;
		}

		if ( ( response->CID != BT_AVDTP_SIGNAL_CID ) || ( remaining < 2 ) )
		{
			continue;
		}

		if ( ( cursor[0] & 15 ) == 0 )
		{
			result = HandlePeerCommand ( session, cursor, remaining, error );

			if ( result != BT_HCI_OK )
			{
				return ( result );
			}

			continue;
		}

		if ( ( ( cursor[0] & 0xf0 ) != label ) || ( ( cursor[0] & 3 ) == 0 ) )
		{
			continue;
		}

		if ( cursor[0] & 0x0c ) // Signaling messages in this subset fit one packet.
		{
			return ( BT_HCI_UNSUPPORTED );
		}

		if ( ( cursor[1] & 0x3f ) != signalID )
		{
			continue;
		}

		if ( ( cursor[0] & 3 ) == 2 )
		{
			return ( BT_HCI_OK );
		}

		session->Source->Request->Status = ( remaining > 2 ) ? cursor[remaining - 1] : 0;

		return ( BT_A2DP_REJECTED );
	}

	return ( BT_HCI_INVALID_REPLY );
}

// --
