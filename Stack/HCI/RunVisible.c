
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_hci.h"

// --

#define HCI_WRITE_LOCAL_NAME		  0x0c13
#define HCI_WRITE_SCAN_ENABLE		  0x0c1a
#define HCI_WRITE_CLASS_OF_DEVICE	  0x0c24
#define HCI_REJECT_CONNECTION_REQUEST 0x040a
#define HCI_EVENT_CONNECTION_REQUEST  0x04
#define HCI_SCAN_DISABLED			  0
#define HCI_SCAN_INQUIRY_AND_PAGE	  3
#define HCI_REJECT_LIMITED_RESOURCES  0x0d

// --

static enum BTHCIResult
WriteScanEnable ( const struct BTHCITransport * transport, uint8_t scan, struct BTHCIError * error )
{
	uint8_t command[4];
	uint8_t reply[1];

	command[0] = HCI_WRITE_SCAN_ENABLE & 0xff;
	command[1] = HCI_WRITE_SCAN_ENABLE >> 8;
	command[2] = 1;
	command[3] = scan;

	return ( BT2_CompleteCommand ( transport, command, sizeof ( command ), reply, sizeof ( reply ), error ) );
}

// --

static enum BTHCIResult
RejectConnection ( const struct BTHCITransport * transport, const uint8_t * address, struct BTHCIError * error )
{
	enum BTHCIResult result;

	uint8_t	 command[10];
	uint8_t	 event[HCI_EVENT_MAX];
	uint32_t byte;
	uint32_t reads;
	uint32_t length;

	command[0]	  = HCI_REJECT_CONNECTION_REQUEST & 0xff;
	command[1]	  = HCI_REJECT_CONNECTION_REQUEST >> 8;
	command[2]	  = 7;
	command[9]	  = HCI_REJECT_LIMITED_RESOURCES;
	error->Opcode = HCI_REJECT_CONNECTION_REQUEST;
	error->Status = 0;
	reads		  = 32;

	for ( byte = 0; byte < 6; byte++ )
	{
		command[3 + byte] = address[byte];
	}

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

		if ( ( event[0] == HCI_EVENT_COMMAND_STATUS ) && ( length == 6 ) && ( event[4] == command[0] )
			 && ( event[5] == command[1] ) )
		{
			error->Status = event[2];

			// A peer can abandon its page before the reject command is sent.
			return ( BT_HCI_OK );
		}
	}

	return ( BT_HCI_INVALID_REPLY );
}

// --

enum BTHCIResult
BT2_RunVisible ( const struct BTHCITransport * transport, const struct BTVisibility * visibility,
				 int ( *expired ) ( void * context ), void * context, struct BTHCIError * error )
{
	struct BTHCIError cleanupError;
	struct BTHCIError resetError;
	enum BTHCIResult  result;
	enum BTHCIResult  cleanup;

	// Class of Device 0x000104 identifies a desktop computer.
	static const uint8_t deviceClass[]
		= { HCI_WRITE_CLASS_OF_DEVICE & 0xff, HCI_WRITE_CLASS_OF_DEVICE >> 8, 3, 0x04, 0x01, 0x00 };
	uint8_t	 name[3 + BT_REMOTE_NAME_MAX];
	uint8_t	 reply[1];
	uint8_t	 event[HCI_EVENT_MAX];
	uint32_t byte;
	uint32_t reads;
	uint32_t length;

	if ( ( ! visibility ) || ( ! visibility->Name[0] ) || ( visibility->Name[BT_REMOTE_NAME_MAX] ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	name[0] = HCI_WRITE_LOCAL_NAME & 0xff;
	name[1] = HCI_WRITE_LOCAL_NAME >> 8;
	name[2] = BT_REMOTE_NAME_MAX;

	for ( byte = 0; ( byte < BT_REMOTE_NAME_MAX ) && ( visibility->Name[byte] ); byte++ )
	{
		name[3 + byte] = visibility->Name[byte];
	}

	while ( byte < BT_REMOTE_NAME_MAX )
	{
		name[3 + byte++] = 0;
	}

	result = BT2_CompleteCommand ( transport, name, sizeof ( name ), reply, sizeof ( reply ), error );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	result = BT2_CompleteCommand ( transport, deviceClass, sizeof ( deviceClass ), reply, sizeof ( reply ), error );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	result = WriteScanEnable ( transport, HCI_SCAN_INQUIRY_AND_PAGE, error );

	if ( result == BT_HCI_OK )
	{
		while ( ! expired ( context ) )
		{
			reads = 64;

			result = BT2_ReadEvent ( transport, event, &length, &reads, error );

			if ( ( result == BT_HCI_READ_ERROR ) && ( error->TransportError == BT_HCI_TRANSPORT_TIMEOUT ) && ( ! length ) )
			{
				result = BT_HCI_OK;

				continue;
			}

			if ( result != BT_HCI_OK )
			{
				break;
			}

			if ( ( event[0] == HCI_EVENT_CONNECTION_REQUEST ) && ( length == 12 ) )
			{
				result = RejectConnection ( transport, &event[2], error );

				if ( result != BT_HCI_OK )
				{
					break;
				}
			}
		}
	}

	cleanup = WriteScanEnable ( transport, HCI_SCAN_DISABLED, &cleanupError );

	if ( cleanup != BT_HCI_OK )
	{
		BT_ResetController ( transport, &resetError );

		if ( result == BT_HCI_OK )
		{
			*error = cleanupError;

			return ( cleanup );
		}
	}

	return ( result );
}

// --
