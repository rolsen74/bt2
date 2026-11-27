
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_hid.h"

// --

enum BTHCIResult
BT2_CheckHIDConnection ( struct BTL2CAPLink * link, struct BTHCIError * error )
{
	const struct BTHCITransport * transport;
	enum BTHCIResult			  result;

	uint8_t	 command[5];
	uint8_t	 event[HCI_EVENT_MAX];
	uint32_t length;
	uint32_t reads;

	transport	  = &link->Transport->HCI;
	error->Opcode = 0x0c36; // Read Link Supervision Timeout.
	error->Status = 0;
	command[0]	  = 0x36;
	command[1]	  = 0x0c;
	command[2]	  = 2;
	command[3]	  = link->Handle & 0xff;
	command[4]	  = link->Handle >> 8;
	reads		  = 64;

	error->TransportError = transport->StartCommand ( transport->Context, command, sizeof ( command ) );

	if ( error->TransportError )
	{
		return ( BT_HCI_WRITE_ERROR );
	}

	while ( reads )
	{
		result = BT2_ReadEvent ( transport, event, &length, &reads, error );

		if ( result != BT_HCI_OK )
		{
			return ( result );
		}

		if ( ( event[0] == 5 ) && ( length == 6 ) && ( ! event[2] ) && ( BT_ReadLE16 ( event + 3 ) == link->Handle ) )
		{
			return ( BT_HID_DISCONNECTED );
		}

		BT2_AccountCompletedPackets ( link, event, length );

		if ( ( event[0] == HCI_EVENT_COMMAND_COMPLETE ) && ( length >= 6 ) && ( event[3] == command[0] )
			 && ( event[4] == command[1] ) )
		{
			error->Status = event[5];

			switch ( event[5] )
			{
				case 0:
					return ( ( ( length == 10 ) && ( BT_ReadLE16 ( event + 6 ) == link->Handle ) ) ? BT_HCI_OK
																								   : BT_HCI_INVALID_REPLY );
				case 1:
					return ( BT_HCI_UNSUPPORTED );
				case 2:
					return ( BT_HID_DISCONNECTED );
				default:
					return ( BT_HCI_COMMAND_ERROR );
			}
		}
	}

	return ( BT_HCI_INVALID_REPLY );
}

// --
