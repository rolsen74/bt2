
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_hci.h"

// --

#define HCI_READ_LOCAL_COMMANDS 0x1002
#define HCI_LE_EVENT 0x3e
#define HCI_LE_ADV_REPORT 0x02

// --

static enum BTHCIResult
ConfigureScan( const struct BTHCITransport *transport, enum BTLEMode mode, struct BTHCIError *error )
{
	enum BTHCIResult result;

	// This worker resets the controller before each operation.
	static const uint8_t eventMask[] = { 0x01, 0x0c, 8, 0, 0, 0, 0, 0, 0, 0, 0x20 };
	static const uint8_t leMask[] = { 0x01, 0x20, 8, 2, 0, 0, 0, 0, 0, 0, 0 };
	uint8_t parameters[] = { 0x0b, 0x20, 7, 0, 0x60, 0, 0x60, 0, 0, 0 };
	uint8_t reply[1];

	parameters[3] = ( mode == BT_LE_SCAN_ACTIVE );

	result = BT2_CompleteCommand( transport, eventMask, sizeof( eventMask ), reply, sizeof( reply ), error );

	if ( result == BT_HCI_OK )
	{
		result = BT2_CompleteCommand( transport, leMask, sizeof( leMask ), reply, sizeof( reply ), error );
	}

	if ( result == BT_HCI_OK )
	{
		result = BT2_CompleteCommand( transport, parameters, sizeof( parameters ), reply, sizeof( reply ), error );
	}

	return ( result );
}

// --

static enum BTHCIResult
ConfigureAdvertising( const struct BTHCITransport *transport, const char *name, struct BTHCIError *error )
{
	enum BTHCIResult result;

	// 100 to 120 ms, nonconnectable, public address, all primary channels.
	static const uint8_t parameters[] = { 0x06, 0x20, 15, 0xa0, 0, 0xc0, 0, 3, 0, 0,
		0, 0, 0, 0, 0, 0, 7, 0 };
	uint8_t data[4 + BT_LE_DATA_MAX] = { 0x08, 0x20, 32 };
	uint8_t reply[1];
	uint32_t length;

	for( length = 0; name[length]; length++ )
	{
		data[6 + length] = name[length];
	}

	data[3] = length + 2;
	data[4] = length + 1;
	data[5] = 9;

	result = BT2_CompleteCommand( transport, parameters, sizeof( parameters ), reply, sizeof( reply ), error );

	if ( result == BT_HCI_OK )
	{
		result = BT2_CompleteCommand( transport, data, sizeof( data ), reply, sizeof( reply ), error );
	}

	return ( result );
}

// --

enum BTHCIResult
BT2_RunLE( const struct BTHCITransport *transport, struct BTLERequest *request,
	int (*expired)( void *context ), void *context, struct BTHCIError *error )
{
	struct BTHCIError cleanupError;
	struct BTHCIError resetError;
	enum BTHCIResult result;
	enum BTHCIResult cleanup;

	uint8_t commands[65];
	uint8_t event[HCI_EVENT_MAX];
	uint8_t enable[5];
	uint8_t reply[1];
	uint32_t reads;
	uint32_t length;
	int scanning;

	if ( ( ! request ) || ( ! expired ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	request->Count = 0;
	request->Truncated = 0;
	scanning = ( request->Mode != BT_LE_ADVERTISE );

	if ( scanning )
	{
		if ( ( ( request->Mode != BT_LE_SCAN_ACTIVE ) && ( request->Mode != BT_LE_SCAN_PASSIVE ) ) ||
			( ( request->Capacity ) && ( ! request->Devices ) ) )
		{
			return ( BT_HCI_INVALID_ARGUMENT );
		}
	}
	else if ( ( ! request->Name[0] ) || ( request->Name[BT_LE_NAME_MAX] ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	if ( __atomic_load_n( & request->Cancel, __ATOMIC_RELAXED ) )
	{
		return ( BT_LE_CANCELLED );
	}

	result = BT_HCICommand( transport, HCI_READ_LOCAL_COMMANDS, commands, sizeof( commands ), error );

	if ( ( result == BT_HCI_COMMAND_ERROR ) && ( error->Status == 1 ) )
	{
		return ( BT_LE_NOT_SUPPORTED );
	}

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	if ( scanning )
	{
		if ( ( ( commands[1 + 26] & 0x0c ) != 0x0c ) || ( ! ( commands[1 + 25] & 1 ) ) )
		{
			return ( BT_LE_NOT_SUPPORTED );
		}

		result = ConfigureScan( transport, request->Mode, error );
	}
	else
	{
		if ( ( ( commands[1 + 25] & 0xa0 ) != 0xa0 ) || ( ! ( commands[1 + 26] & 2 ) ) )
		{
			return ( BT_LE_NOT_SUPPORTED );
		}

		result = ConfigureAdvertising( transport, request->Name, error );
	}

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	enable[0] = scanning ? 0x0c : 0x0a;
	enable[1] = 0x20;
	enable[2] = scanning ? 2 : 1;
	enable[3] = 1;
	enable[4] = 1;

	result = BT2_CompleteCommand( transport, enable, 3 + enable[2], reply, sizeof( reply ), error );

	while( ( result == BT_HCI_OK ) && ( ! expired( context ) ) )
	{
		if ( __atomic_load_n( & request->Cancel, __ATOMIC_RELAXED ) )
		{
			result = BT_LE_CANCELLED;

			break;
		}

		reads = 64;

		result = BT2_ReadEvent( transport, event, & length, & reads, error );

		if ( ( result == BT_HCI_READ_ERROR ) && ( error->TransportError == BT_HCI_TRANSPORT_TIMEOUT ) && ( ! length ) )
		{
			result = BT_HCI_OK;

			continue;
		}

		if ( ( result == BT_HCI_OK ) && ( scanning ) && ( length >= 3 ) &&
			( event[0] == HCI_LE_EVENT ) && ( event[2] == HCI_LE_ADV_REPORT ) )
		{
			if ( ! BT2_ParseLEReports( request, event, length ) )
			{
				result = BT_HCI_INVALID_REPLY;
			}
		}
	}

	enable[3] = 0;
	enable[4] = 0;

	cleanup = BT2_CompleteCommand( transport, enable, 3 + enable[2], reply, sizeof( reply ), & cleanupError );

	if ( cleanup != BT_HCI_OK )
	{
		BT_ResetController( transport, & resetError );

		if ( ( result == BT_HCI_OK ) || ( result == BT_LE_CANCELLED ) )
		{
			*error = cleanupError;

			return ( cleanup );
		}
	}

	return ( result );
}

// --
