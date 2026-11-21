
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <stddef.h>
#include "bt2_sdp.h"

// --

#define SDP_PSM 0x0001
#define SDP_LOCAL_CID 0x0040
#define SDP_ERROR_RESPONSE 0x01
#define SDP_SERVICE_SEARCH_ATTR_REQUEST 0x06
#define SDP_SERVICE_SEARCH_ATTR_RESPONSE 0x07
#define SDP_HEADER_LENGTH 5
#define SDP_CONTINUATION_MAX 16
#define SDP_MAX_ATTRIBUTE_BYTES 600

// --

static enum BTHCIResult
ReadResponse( struct BTL2CAPLink *link, uint16_t transaction, struct BTL2CAPPacket *response, struct BTHCIError *error )
{
	struct BTL2CAPSignal signal;
	enum BTHCIResult result;

	const uint8_t *data;
	uint32_t length;
	uint32_t reads;

	reads = 256;

	while( reads )
	{
		result = BT2_ReadL2CAP( link, response, & reads, error );

		if ( result != BT_HCI_OK )
		{
			return ( result );
		}

		data = response->Data + BT_L2CAP_HEADER;

		if ( response->CID == BT_L2CAP_SIGNAL_CID )
		{
			length = response->Length;

			while( BT2_DecodeSignal( & data, & length, & signal ) )
			{
				result = BT2_ServiceSignal( link, & signal, error );

				if ( result != BT_HCI_OK )
				{
					return ( result );
				}
			}

			continue;
		}

		if ( ( response->CID == SDP_LOCAL_CID ) && ( response->Length >= SDP_HEADER_LENGTH ) &&
			( BT_ReadBE16( data + 1 ) == transaction ) )
		{
			if ( BT_ReadBE16( data + 3 ) != response->Length - SDP_HEADER_LENGTH )
			{
				return ( BT_HCI_INVALID_REPLY );
			}

			return ( BT_HCI_OK );
		}
	}

	return ( BT_HCI_INVALID_REPLY );
}

// --

static enum BTHCIResult
SearchAttributes( struct BTL2CAPLink *link, uint16_t remoteCID, struct BTServiceQuery *query,
	uint16_t uuid, struct BTHCIError *error )
{
	struct BTL2CAPPacket response;
	enum BTHCIResult result;

	// Search UUID, followed by handle, class, protocol list, and name attribute IDs.
	static const uint8_t parameters[] = { 0x35, 3, 0x19, 0, 0, SDP_MAX_ATTRIBUTE_BYTES >> 8,
		SDP_MAX_ATTRIBUTE_BYTES & 0xff, 0x35, 12, 0x09, 0, 0, 0x09, 0, 1, 0x09, 0, 4, 0x09, 1, 0 };
	const uint8_t *data;
	uint8_t request[SDP_HEADER_LENGTH + sizeof( parameters ) + 1 + SDP_CONTINUATION_MAX];
	uint8_t attributes[8192];
	uint32_t byte;
	uint32_t length;
	uint32_t used;
	uint32_t count;
	uint32_t stateLength;
	uint16_t transaction;

	used = 0;
	stateLength = 0;
	request[0] = SDP_SERVICE_SEARCH_ATTR_REQUEST;

	for( byte = 0; byte < sizeof( parameters ); byte++ )
	{
		request[SDP_HEADER_LENGTH + byte] = parameters[byte];
	}

	request[SDP_HEADER_LENGTH + 3] = uuid >> 8;
	request[SDP_HEADER_LENGTH + 4] = uuid & 0xff;

	for( transaction = 1; transaction <= 32; transaction++ )
	{
		length = sizeof( parameters ) + 1 + stateLength;
		request[1] = transaction >> 8;
		request[2] = transaction & 0xff;
		request[3] = length >> 8;
		request[4] = length & 0xff;
		request[SDP_HEADER_LENGTH + sizeof( parameters )] = stateLength;

		result = BT2_WriteL2CAP( link, remoteCID, request, SDP_HEADER_LENGTH + length, error );

		if ( result != BT_HCI_OK )
		{
			return ( result );
		}

		result = ReadResponse( link, transaction, & response, error );

		if ( result != BT_HCI_OK )
		{
			return ( result );
		}

		data = response.Data + BT_L2CAP_HEADER;
		length = response.Length - SDP_HEADER_LENGTH;

		if ( ( data[0] == SDP_ERROR_RESPONSE ) && ( length == 2 ) )
		{
			query->Status = BT_ReadBE16( data + SDP_HEADER_LENGTH );

			return ( BT_SDP_ERROR );
		}

		if ( ( data[0] != SDP_SERVICE_SEARCH_ATTR_RESPONSE ) || ( length < 3 ) )
		{
			return ( BT_HCI_INVALID_REPLY );
		}

		data += SDP_HEADER_LENGTH;
		count = BT_ReadBE16( data );

		if ( ( count > length - 3 ) || ( count > sizeof( attributes ) - used ) ||
			( count > SDP_MAX_ATTRIBUTE_BYTES ) )
		{
			return ( BT_HCI_INVALID_REPLY );
		}

		stateLength = data[2 + count];

		if ( ( stateLength > SDP_CONTINUATION_MAX ) || ( stateLength != length - 3 - count ) ||
			( ( stateLength ) && ( ! count ) ) )
		{
			return ( BT_HCI_INVALID_REPLY );
		}

		for( byte = 0; byte < count; byte++ )
		{
			attributes[used + byte] = data[2 + byte];
		}

		used += count;

		if ( ! stateLength )
		{
			if ( ! used )
			{
				return ( BT_HCI_OK );
			}

			return ( BT2_ParseServiceRecords( query, attributes, used ) );
		}

		for( byte = 0; byte < stateLength; byte++ )
		{
			request[SDP_HEADER_LENGTH + sizeof( parameters ) + 1 + byte] = data[3 + count + byte];
		}
	}

	return ( BT_HCI_INVALID_REPLY );
}

// --

enum BTHCIResult
BT2_SearchServices( const struct BTACLTransport *transport, struct BTServiceQuery *query,
	uint16_t uuid, const uint32_t *cancel, struct BTHCIError *error )
{
	struct BTL2CAPLink link;
	enum BTHCIResult result;

	uint16_t remoteCID;

	if ( ( ! query ) || ( ( query->Capacity ) && ( ! query->Records ) ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	query->Count = 0;
	query->Status = 0;
	query->Truncated = 0;

	if ( ( cancel ) && ( __atomic_load_n( cancel, __ATOMIC_RELAXED ) ) )
	{
		return ( BT_RFCOMM_CANCELLED );
	}

	result = BT2_OpenDataLink( & link, transport, query->Address, error );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	link.Cancel = cancel;

	result = BT2_OpenChannel( & link, SDP_PSM, SDP_LOCAL_CID, & remoteCID, error );

	if ( result == BT_HCI_OK )
	{
		result = SearchAttributes( & link, remoteCID, query, uuid, error );
	}

	return ( BT2_CloseDataLink( & link, result, error ) );
}

// --

enum BTHCIResult
BT_BrowseServices( const struct BTACLTransport *transport, struct BTServiceQuery *query, struct BTHCIError *error )
{
	return ( BT2_SearchServices( transport, query, 0x1002, NULL, error ) );
}

// --
