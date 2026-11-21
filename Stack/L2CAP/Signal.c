
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

enum BTHCIResult
BT2_WriteSignal( struct BTL2CAPLink *link, uint8_t code, uint8_t id,
	const uint8_t *data, uint8_t length, struct BTHCIError *error )
{
	uint8_t signal[BT_L2CAP_SIGNAL_HEADER + 255];
	uint32_t byte;

	signal[0] = code;
	signal[1] = id;
	signal[2] = length;
	signal[3] = 0;

	for( byte = 0; byte < length; byte++ )
	{
		signal[BT_L2CAP_SIGNAL_HEADER + byte] = data[byte];
	}

	return ( BT2_WriteL2CAP( link, BT_L2CAP_SIGNAL_CID, signal, BT_L2CAP_SIGNAL_HEADER + length, error ) );
}

// --

int
BT2_DecodeSignal( const uint8_t **cursor, uint32_t *remaining, struct BTL2CAPSignal *signal )
{
	uint16_t length;

	if ( *remaining < BT_L2CAP_SIGNAL_HEADER )
	{
		return ( 0 );
	}

	length = BT_ReadLE16( *cursor + 2 );

	if ( length > *remaining - BT_L2CAP_SIGNAL_HEADER )
	{
		return ( 0 );
	}

	signal->Code = (*cursor)[0];
	signal->ID = (*cursor)[1];
	signal->Length = length;
	signal->Data = *cursor + BT_L2CAP_SIGNAL_HEADER;
	*cursor += BT_L2CAP_SIGNAL_HEADER + length;
	*remaining -= BT_L2CAP_SIGNAL_HEADER + length;

	return ( 1 );
}

// --

enum BTHCIResult
BT2_ServiceSignal( struct BTL2CAPLink *link, const struct BTL2CAPSignal *signal, struct BTHCIError *error )
{
	uint8_t response[8];
	uint8_t length;

	if ( ! signal->ID )
	{
		return ( BT_HCI_OK );
	}

	// The outgoing profiles do not listen for incoming L2CAP channels.
	if ( ( signal->Code == 0x02 ) && ( signal->Length == 4 ) )
	{
		response[0] = 0;
		response[1] = 0;
		response[2] = signal->Data[2];
		response[3] = signal->Data[3];
		response[4] = 2; // PSM not supported.
		response[5] = 0;
		response[6] = 0;
		response[7] = 0;

		return ( BT2_WriteSignal( link, 0x03, signal->ID, response, sizeof( response ), error ) );
	}

	if ( ( signal->Code == 0x0a ) && ( signal->Length == 2 ) )
	{
		response[0] = signal->Data[0];
		response[1] = signal->Data[1];
		response[2] = 1; // Information type not supported.
		response[3] = 0;
		length = 4;

		if ( BT_ReadLE16( signal->Data ) == 2 )
		{
			// Extended features mask zero advertises Basic Mode only.
			response[2] = 0;
			response[4] = 0;
			response[5] = 0;
			response[6] = 0;
			response[7] = 0;
			length = 8;
		}

		return ( BT2_WriteSignal( link, 0x0b, signal->ID, response, length, error ) );
	}

	if ( signal->Code == 0x08 )
	{
		return ( BT2_WriteSignal( link, 0x09, signal->ID, NULL, 0, error ) );
	}

	return ( BT_HCI_OK );
}

// --
