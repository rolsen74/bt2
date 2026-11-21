
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_hci.h"

// --

enum BTHCIResult
BT2_ReadEvent( const struct BTHCITransport *transport, uint8_t *event,
	uint32_t *length, uint32_t *reads, struct BTHCIError *error )
{
	const uint8_t *chunk;
	uint32_t chunkLength;
	uint32_t total;
	uint32_t byte;

	*length = 0;
	total = HCI_EVENT_MAX;

	while( *reads )
	{
		(*reads)--;

		error->TransportError = transport->ReadEventChunk( transport->Context, & chunk, & chunkLength );

		if ( error->TransportError )
		{
			return ( BT_HCI_READ_ERROR );
		}

		if ( chunkLength > total - *length )
		{
			break;
		}

		for( byte = 0; byte < chunkLength; byte++ )
		{
			event[*length + byte] = chunk[byte];
		}

		*length += chunkLength;

		if ( *length >= HCI_EVENT_HEADER )
		{
			total = HCI_EVENT_HEADER + event[1];

			if ( *length == total )
			{
				return ( BT_HCI_OK );
			}

			if ( *length > total )
			{
				break;
			}
		}
	}

	return ( BT_HCI_INVALID_REPLY );
}

// --
