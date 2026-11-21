
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_hci.h"

// --

#define HCI_INQUIRY 0x0401
#define HCI_INQUIRY_CANCEL 0x0402
#define HCI_EVENT_INQUIRY_COMPLETE 0x01
#define HCI_EVENT_INQUIRY_RESULT 0x02
#define HCI_INQUIRY_RESPONSE_SIZE 14U

// --

static void
AddResponse( struct BTInquiryResult *inquiry, const uint8_t *response )
{
	struct BTInquiryDevice *device;

	uint32_t index;
	uint32_t byte;

	for( index = 0; index < inquiry->Count; index++ )
	{
		device = & inquiry->Devices[index];

		for( byte = 0; byte < sizeof( device->Address ); byte++ )
		{
			if ( device->Address[byte] != response[5 - byte] )
			{
				break;
			}
		}

		if ( byte == sizeof( device->Address ) )
		{
			return;
		}
	}

	if ( inquiry->Count == inquiry->Capacity )
	{
		inquiry->Truncated = 1;

		return;
	}

	device = & inquiry->Devices[inquiry->Count++];

	for( byte = 0; byte < sizeof( device->Address ); byte++ )
	{
		device->Address[byte] = response[5 - byte];
	}

	device->PageScanRepetitionMode = response[6];
	device->ClassOfDevice = response[9] | ( (uint32_t) response[10] << 8 ) | ( (uint32_t) response[11] << 16 );
	device->ClockOffset = ( response[12] | ( (uint16_t) response[13] << 8 ) ) & 0x7fff;
}

// --

enum BTHCIResult
BT_Inquiry( const struct BTHCITransport *transport, struct BTInquiryResult *inquiry, struct BTHCIError *error )
{
	struct BTHCIError cancelError;
	enum BTHCIResult result;

	// General Inquiry Access Code, four 1.28 s periods, unlimited responses.
	static const uint8_t command[] = { HCI_INQUIRY & 0xff, HCI_INQUIRY >> 8, 5, 0x33, 0x8b, 0x9e, 4, 0 };
	uint8_t event[HCI_EVENT_MAX];
	uint8_t cancelReply[1];
	uint32_t reads;
	uint32_t timeouts;
	uint32_t length;
	uint32_t index;

	inquiry->Count = 0;
	inquiry->Truncated = 0;
	error->Opcode = HCI_INQUIRY;
	error->TransportError = 0;
	error->Status = 0;
	result = BT_HCI_INVALID_REPLY;
	reads = 4096;
	timeouts = 0;

	error->TransportError = transport->StartCommand( transport->Context, command, sizeof( command ) );

	if ( error->TransportError )
	{
		return ( BT_HCI_WRITE_ERROR );
	}

	while( reads )
	{
		result = BT2_ReadEvent( transport, event, & length, & reads, error );

		if ( result != BT_HCI_OK )
		{
			if ( ( result == BT_HCI_READ_ERROR ) && ( error->TransportError == BT_HCI_TRANSPORT_TIMEOUT ) &&
				( ! length ) && ( ++timeouts < 6 ) )
			{
				continue;
			}

			break;
		}

		if ( ( event[0] == HCI_EVENT_COMMAND_STATUS ) && ( length == 6 ) &&
			( event[4] == command[0] ) && ( event[5] == command[1] ) && ( event[2] ) )
		{
			error->Status = event[2];

			return ( BT_HCI_COMMAND_ERROR );
		}

		if ( ( event[0] == HCI_EVENT_INQUIRY_COMPLETE ) && ( length == 3 ) )
		{
			error->Status = event[2];

			return ( error->Status ? BT_HCI_COMMAND_ERROR : BT_HCI_OK );
		}

		if ( event[0] == HCI_EVENT_INQUIRY_RESULT )
		{
			if ( ( length < 3 ) || ( length != 3 + event[2] * HCI_INQUIRY_RESPONSE_SIZE ) )
			{
				result = BT_HCI_INVALID_REPLY;

				break;
			}

			for( index = 0; index < event[2]; index++ )
			{
				AddResponse( inquiry, & event[3 + index * HCI_INQUIRY_RESPONSE_SIZE] );
			}
		}
	}

	BT_HCICommand( transport, HCI_INQUIRY_CANCEL, cancelReply, sizeof( cancelReply ), & cancelError );

	return ( result == BT_HCI_OK ? BT_HCI_INVALID_REPLY : result );
}

// --
