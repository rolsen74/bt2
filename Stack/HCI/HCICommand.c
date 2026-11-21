
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_hci.h"

// --

#define HCI_COMMAND_COMPLETE_HEADER 5

// --

enum BTHCIResult
BT_HCICommand( const struct BTHCITransport *transport, uint16_t opcode,
	uint8_t *reply, uint32_t replyLength, struct BTHCIError *error )
{
	enum BTHCIResult result;

	uint8_t command[3];
	uint8_t event[HCI_EVENT_MAX];
	uint32_t reads;
	uint32_t total;
	uint32_t byte;

	command[0] = opcode & 0xff;
	command[1] = opcode >> 8;
	command[2] = 0;
	error->Opcode = opcode;
	error->TransportError = 0;
	error->Status = 0;
	reads = 64;

	error->TransportError = transport->StartCommand( transport->Context, command, sizeof( command ) );

	if ( error->TransportError )
	{
		return ( BT_HCI_WRITE_ERROR );
	}

	while( reads )
	{
		result = BT2_ReadEvent( transport, event, & total, & reads, error );

		if ( result != BT_HCI_OK )
		{
			return ( result );
		}

		if ( ( event[0] == HCI_EVENT_COMMAND_COMPLETE ) && ( total > HCI_COMMAND_COMPLETE_HEADER ) &&
			( ( ( (uint16_t) event[4] << 8 ) | event[3] ) == opcode ) )
		{
			error->Status = event[5];

			if ( error->Status )
			{
				return ( BT_HCI_COMMAND_ERROR );
			}

			if ( total - HCI_COMMAND_COMPLETE_HEADER != replyLength )
			{
				break;
			}

			for( byte = 0; byte < replyLength; byte++ )
			{
				reply[byte] = event[HCI_COMMAND_COMPLETE_HEADER + byte];
			}

			return ( BT_HCI_OK );
		}

		if ( ( event[0] == HCI_EVENT_COMMAND_STATUS ) && ( total == 6 ) && ( event[2] ) &&
			( ( ( (uint16_t) event[5] << 8 ) | event[4] ) == opcode ) )
		{
			error->Status = event[2];

			return ( BT_HCI_COMMAND_ERROR );
		}
	}

	return ( BT_HCI_INVALID_REPLY );
}

// --
