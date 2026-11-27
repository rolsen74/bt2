
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_hci.h"

// --

#define HCI_REMOTE_NAME_REQUEST		   0x0419
#define HCI_REMOTE_NAME_REQUEST_CANCEL 0x041a
#define HCI_EVENT_REMOTE_NAME_COMPLETE 0x07
#define HCI_REMOTE_NAME_HEADER		   9

// --

enum BTHCIResult
BT_ReadRemoteName ( const struct BTHCITransport * transport, struct BTRemoteName * remote, struct BTHCIError * error )
{
	struct BTHCIError cancelError;
	enum BTHCIResult  result;

	uint8_t	 command[13];
	uint8_t	 event[HCI_EVENT_MAX];
	uint8_t	 cancelReply[7];
	uint32_t reads;
	uint32_t timeouts;
	uint32_t length;
	uint32_t byte;

	if ( ! remote )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	remote->Name[0] = 0;
	error->Opcode	= HCI_REMOTE_NAME_REQUEST;
	error->Status	= 0;
	command[0]		= HCI_REMOTE_NAME_REQUEST & 0xff;
	command[1]		= HCI_REMOTE_NAME_REQUEST >> 8;
	command[2]		= 10;

	for ( byte = 0; byte < 6; byte++ )
	{
		command[3 + byte] = remote->Address[5 - byte];
	}

	// Use R2 without a cached page-scan mode or clock offset.
	command[9]	= 2;
	command[10] = 0;
	command[11] = 0;
	command[12] = 0;

	error->TransportError = transport->StartCommand ( transport->Context, command, sizeof ( command ) );

	if ( error->TransportError )
	{
		return ( BT_HCI_WRITE_ERROR );
	}

	reads	 = 256;
	timeouts = 0;
	result	 = BT_HCI_INVALID_REPLY;

	while ( reads )
	{
		result = BT2_ReadEvent ( transport, event, &length, &reads, error );

		if ( result != BT_HCI_OK )
		{
			if ( ( result == BT_HCI_READ_ERROR ) && ( error->TransportError == BT_HCI_TRANSPORT_TIMEOUT ) && ( ! length )
				 && ( ++timeouts < 12 ) )
			{
				continue;
			}

			break;
		}

		if ( ( event[0] == HCI_EVENT_COMMAND_STATUS ) && ( length == 6 ) && ( event[4] == command[0] )
			 && ( event[5] == command[1] ) && ( event[2] ) )
		{
			error->Status = event[2];

			return ( BT_HCI_COMMAND_ERROR );
		}

		if ( ( event[0] != HCI_EVENT_REMOTE_NAME_COMPLETE ) || ( length < HCI_REMOTE_NAME_HEADER ) )
		{
			continue;
		}

		for ( byte = 0; byte < 6; byte++ )
		{
			if ( event[3 + byte] != command[3 + byte] )
			{
				break;
			}
		}

		if ( byte != 6 )
		{
			continue;
		}

		error->Status = event[2];

		if ( error->Status )
		{
			return ( BT_HCI_COMMAND_ERROR );
		}

		if ( length != HCI_REMOTE_NAME_HEADER + BT_REMOTE_NAME_MAX )
		{
			return ( BT_HCI_INVALID_REPLY );
		}

		for ( byte = 0; byte < BT_REMOTE_NAME_MAX; byte++ )
		{
			if ( ! event[HCI_REMOTE_NAME_HEADER + byte] )
			{
				break;
			}

			remote->Name[byte] = event[HCI_REMOTE_NAME_HEADER + byte];
		}

		remote->Name[byte] = 0;

		return ( BT_HCI_OK );
	}

	command[0] = HCI_REMOTE_NAME_REQUEST_CANCEL & 0xff;
	command[1] = HCI_REMOTE_NAME_REQUEST_CANCEL >> 8;
	command[2] = 6;

	BT2_CompleteCommand ( transport, command, 9, cancelReply, sizeof ( cancelReply ), &cancelError );

	return ( result == BT_HCI_OK ? BT_HCI_INVALID_REPLY : result );
}

// --
