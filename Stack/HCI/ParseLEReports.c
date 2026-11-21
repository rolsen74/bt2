
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_hci.h"

// --

#define LE_REPORT_HEADER 9
#define LE_SCAN_RESPONSE 4
#define LE_SHORT_NAME 8
#define LE_COMPLETE_NAME 9

// --

static void
ReadName( struct BTLEDevice *device, const uint8_t *data, uint32_t length )
{
	uint32_t field;
	uint32_t byte;
	uint8_t type;

	while( length )
	{
		field = *data;

		if ( ( ! field ) || ( field >= length ) )
		{
			break;
		}

		type = data[1];

		if ( ( field > 1 ) &&
			( ( type == LE_COMPLETE_NAME ) || ( ( type == LE_SHORT_NAME ) && ( ! device->CompleteName ) ) ) )
		{
			for( byte = 0; byte < field - 1; byte++ )
			{
				device->Name[byte] = data[2 + byte];
			}

			device->Name[byte] = 0;
			device->CompleteName = ( type == LE_COMPLETE_NAME );
		}

		data += 1 + field;
		length -= 1 + field;
	}
}

// --

static void
SaveReport( struct BTLERequest *request, const uint8_t *report )
{
	struct BTLEDevice *device;

	volatile uint8_t *record;
	uint8_t *data;
	uint32_t index;
	uint32_t byte;
	uint32_t size;
	uint8_t rssi;

	size = report[8];

	for( index = 0; index < request->Count; index++ )
	{
		device = & request->Devices[index];

		if ( device->AddressType != report[1] )
		{
			continue;
		}

		for( byte = 0; ( byte < 6 ) && ( device->Address[byte] == report[7 - byte] ); byte++ )
		{
		}

		if ( byte == 6 )
		{
			break;
		}
	}

	if ( index == request->Count )
	{
		if ( request->Count == request->Capacity )
		{
			request->Truncated = 1;

			return;
		}

		device = & request->Devices[request->Count++];
		record = (volatile uint8_t *) device;

		for( byte = 0; byte < sizeof( *device ); byte++ )
		{
			record[byte] = 0;
		}

		device->AddressType = report[1];
		device->RSSI = BT_LE_RSSI_UNKNOWN;
		device->AdvertisingType = LE_SCAN_RESPONSE;

		for( byte = 0; byte < 6; byte++ )
		{
			device->Address[byte] = report[7 - byte];
		}
	}

	rssi = report[LE_REPORT_HEADER + size];

	if ( rssi != BT_LE_RSSI_UNKNOWN )
	{
		device->RSSI = ( rssi < 128 ) ? rssi : (int32_t) rssi - 256;
	}

	if ( report[0] == LE_SCAN_RESPONSE )
	{
		device->ScanResponseLength = size;
		data = device->ScanResponseData;
	}
	else
	{
		device->AdvertisingType = report[0];
		device->AdvertisingLength = size;
		data = device->AdvertisingData;
	}

	for( byte = 0; byte < size; byte++ )
	{
		data[byte] = report[LE_REPORT_HEADER + byte];
	}

	ReadName( device, data, size );
}

// --

int
BT2_ParseLEReports( struct BTLERequest *request, const uint8_t *event, uint32_t length )
{
	uint32_t count;
	uint32_t index;
	uint32_t offset;
	uint32_t size;

	if ( length < 4 )
	{
		return ( 0 );
	}

	count = event[3];
	offset = 4;

	// Check all report boundaries before updating the results.
	for( index = 0; index < count; index++ )
	{
		if ( length - offset < LE_REPORT_HEADER + 1 )
		{
			return ( 0 );
		}

		size = event[offset + 8];

		if ( ( size > BT_LE_DATA_MAX ) || ( length - offset < LE_REPORT_HEADER + 1 + size ) )
		{
			return ( 0 );
		}

		offset += LE_REPORT_HEADER + 1 + size;
	}

	if ( ( ! count ) || ( offset != length ) )
	{
		return ( 0 );
	}

	offset = 4;

	for( index = 0; index < count; index++ )
	{
		SaveReport( request, & event[offset] );

		offset += LE_REPORT_HEADER + 1 + event[offset + 8];
	}

	return ( 1 );
}

// --
