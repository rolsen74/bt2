
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_sdp.h"

// --

#define SDP_UINT	 1
#define SDP_UUID	 3
#define SDP_TEXT	 4
#define SDP_SEQUENCE 6

// RFCOMMChannel occupies one byte of the original record padding.
_Static_assert ( sizeof ( struct BTServiceRecord ) == 88, "BTServiceRecord ABI size" );

struct SDPElement
{
	const uint8_t * Data;
	uint32_t		Length;
	uint8_t			Type;
};

// --

static int
ReadElement ( const uint8_t ** data, uint32_t * remaining, struct SDPElement * element )
{
	const uint8_t * header;
	uint32_t		size;
	uint32_t		length;
	uint32_t		headerLength;
	uint32_t		byte;

	if ( ! *remaining )
	{
		return ( 0 );
	}

	header		  = *data;
	element->Type = header[0] >> 3;
	size		  = header[0] & 7;
	headerLength  = 1;

	if ( ! element->Type )
	{
		if ( size )
		{
			return ( 0 );
		}

		length = 0;
	}
	else if ( size < 5 )
	{
		length = 1U << size;
	}
	else
	{
		headerLength += 1U << ( size - 5 );

		if ( headerLength > *remaining )
		{
			return ( 0 );
		}

		length = 0;

		for ( byte = 1; byte < headerLength; byte++ )
		{
			length = ( length << 8 ) | header[byte];
		}
	}

	if ( length > *remaining - headerLength )
	{
		return ( 0 );
	}

	element->Data	= header + headerLength;
	element->Length = length;
	*data += headerLength + length;
	*remaining -= headerLength + length;

	return ( 1 );
}

// --

static uint8_t
ReadRFCOMMChannel ( const struct SDPElement * value )
{
	struct SDPElement protocol;
	struct SDPElement uuid;
	struct SDPElement channel;

	const uint8_t * data;
	const uint8_t * entry;
	uint32_t		remaining;
	uint32_t		entryLength;

	if ( value->Type != SDP_SEQUENCE )
	{
		return ( 0 );
	}

	data	  = value->Data;
	remaining = value->Length;

	while ( ReadElement ( &data, &remaining, &protocol ) )
	{
		if ( protocol.Type != SDP_SEQUENCE )
		{
			continue;
		}

		entry		= protocol.Data;
		entryLength = protocol.Length;

		if ( ( ReadElement ( &entry, &entryLength, &uuid ) ) && ( uuid.Type == SDP_UUID ) && ( uuid.Length == 2 )
			 && ( BT_ReadBE16 ( uuid.Data ) == 0x0003 ) && ( ReadElement ( &entry, &entryLength, &channel ) )
			 && ( channel.Type == SDP_UINT ) && ( channel.Length == 1 ) && ( channel.Data[0] >= 1 )
			 && ( channel.Data[0] <= 30 ) )
		{
			return ( channel.Data[0] );
		}
	}

	return ( 0 );
}

// --

static int
ReadAttributes ( struct BTServiceRecord * record, const struct SDPElement * attributes )
{
	struct SDPElement id;
	struct SDPElement value;
	struct SDPElement uuid;

	const uint8_t * data;
	const uint8_t * classes;
	uint32_t		remaining;
	uint32_t		classBytes;
	uint32_t		byte;
	uint16_t		attribute;

	data				  = attributes->Data;
	remaining			  = attributes->Length;
	record->Handle		  = 0;
	record->UUIDLength	  = 0;
	record->Name[0]		  = 0;
	record->RFCOMMChannel = 0;

	while ( remaining )
	{
		if ( ( ! ReadElement ( &data, &remaining, &id ) ) || ( id.Type != SDP_UINT ) || ( id.Length != 2 )
			 || ( ! ReadElement ( &data, &remaining, &value ) ) )
		{
			return ( 0 );
		}

		attribute = BT_ReadBE16 ( id.Data );

		switch ( attribute )
		{
			case 0x0000:
				if ( ( value.Type != SDP_UINT ) || ( value.Length != 4 ) )
				{
					return ( 0 );
				}

				record->Handle = ( (uint32_t)BT_ReadBE16 ( value.Data ) << 16 ) | BT_ReadBE16 ( value.Data + 2 );

				break;

			case 0x0001:
				classes	   = value.Data;
				classBytes = value.Length;

				if ( ( value.Type != SDP_SEQUENCE ) || ( ! ReadElement ( &classes, &classBytes, &uuid ) )
					 || ( uuid.Type != SDP_UUID ) || ( ( uuid.Length != 2 ) && ( uuid.Length != 4 ) && ( uuid.Length != 16 ) ) )
				{
					return ( 0 );
				}

				for ( byte = 0; byte < uuid.Length; byte++ )
				{
					record->UUID[byte] = uuid.Data[byte];
				}

				record->UUIDLength = uuid.Length;

				break;

			case 0x0004:
				record->RFCOMMChannel = ReadRFCOMMChannel ( &value );

				break;

			case 0x0100:
				if ( value.Type != SDP_TEXT )
				{
					break;
				}

				for ( byte = 0; ( byte < value.Length ) && ( byte < sizeof ( record->Name ) - 1 ); byte++ )
				{
					record->Name[byte] = value.Data[byte];
				}

				record->Name[byte] = 0;

				break;
		}
	}

	return ( 1 );
}

// --

enum BTHCIResult
BT2_ParseServiceRecords ( struct BTServiceQuery * query, const uint8_t * data, uint32_t length )
{
	struct SDPElement		 root;
	struct SDPElement		 attributes;
	struct BTServiceRecord	 discarded;
	struct BTServiceRecord * record;

	if ( ( ! ReadElement ( &data, &length, &root ) ) || ( root.Type != SDP_SEQUENCE ) || ( length ) )
	{
		return ( BT_HCI_INVALID_REPLY );
	}

	data   = root.Data;
	length = root.Length;

	while ( length )
	{
		if ( ( ! ReadElement ( &data, &length, &attributes ) ) || ( attributes.Type != SDP_SEQUENCE ) )
		{
			return ( BT_HCI_INVALID_REPLY );
		}

		record = query->Count < query->Capacity ? &query->Records[query->Count] : &discarded;

		if ( ! ReadAttributes ( record, &attributes ) )
		{
			return ( BT_HCI_INVALID_REPLY );
		}

		if ( record == &discarded )
		{
			query->Truncated = 1;
		}
		else
		{
			query->Count++;
		}
	}

	return ( BT_HCI_OK );
}

// --
