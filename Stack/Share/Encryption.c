
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_share.h"

// --

S32
BT2_ShareEncrypted ( struct BTL2CAPLink * link, struct BTHCIError * error )
{
	const struct BTHCITransport * transport;
	enum BTHCIResult			  result;

	uint8_t command[] = { 0x08, 0x14, 2, 0, 0 };
	uint8_t event[HCI_EVENT_MAX];
	U32		length;
	U32		reads;

	// Read Encryption Key Size fails when encryption is disabled.
	transport  = &link->Transport->HCI;
	command[3] = link->Handle;
	command[4] = link->Handle >> 8;
	reads	   = 64;

	error->TransportError = transport->StartCommand ( transport->Context, command, sizeof ( command ) );

	if ( error->TransportError )
	{
		return ( FALSE );
	}

	while ( reads )
	{
		result = BT2_ReadEvent ( transport, event, &length, &reads, error );

		if ( result != BT_HCI_OK )
		{
			return ( FALSE );
		}

		if ( ( event[0] == HCI_EVENT_COMMAND_COMPLETE ) && ( length == 9 ) && ( BT_ReadLE16 ( event + 3 ) == 0x1408 ) )
		{
			return ( ( ! event[5] ) && ( BT_ReadLE16 ( event + 6 ) == link->Handle ) && ( event[8] >= 7 ) );
		}

		if ( ( event[0] == 5 ) && ( length == 6 ) && ( BT_ReadLE16 ( event + 3 ) == link->Handle ) )
		{
			return ( FALSE );
		}

		BT2_AccountCompletedPackets ( link, event, length );
	}

	return ( FALSE );
}

// --
