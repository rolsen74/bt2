
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_share.h"
#include "bt2_sdp.h"

// --

#define SHARE_RECORD 0x00010000

// Attribute pairs for the Classic FTP 1.1 (RFCOMM) service record.
static const uint8_t Record[] = {
	0x09, 0x00, 0x00, 0x0a, 0x00, 0x01, 0x00, 0x00, 0x09, 0x00, 0x01,
	0x35, 0x03, 0x19, 0x11, 0x06, 0x09, 0x00, 0x04, 0x35, 0x11, 0x35,
	0x03, 0x19, 0x01, 0x00, 0x35, 0x05, 0x19, 0x00, 0x03, 0x08, BT_SHARE_CHANNEL,
	0x35, 0x03, 0x19, 0x00, 0x08, 0x09, 0x00, 0x05, 0x35, 0x03, 0x19,
	0x10, 0x02, 0x09, 0x00, 0x09, 0x35, 0x08, 0x35, 0x06, 0x19, 0x11,
	0x06, 0x09, 0x01, 0x01, 0x09, 0x01, 0x00, 0x25, 0x0a, 'F',	'i',
	'l',  'e',	' ',  'S',	'h',  'a',	'r',  'e',
};

static const uint8_t AttributeSizes[] = { 8, 8, 22, 8, 13, 15 };

// --

static S32
Sequence ( const uint8_t ** data, U32 * length, U32 * size )
{
	const uint8_t * cursor;

	cursor = *data;

	if ( ( *length >= 2 ) && ( cursor[0] == 0x35 ) )
	{
		*size = cursor[1];
		*data += 2;
		*length -= 2;
	}
	else if ( ( *length >= 3 ) && ( cursor[0] == 0x36 ) )
	{
		*size = BT_ReadBE16 ( cursor + 1 );

		*data += 3;
		*length -= 3;
	}
	else
	{
		return ( FALSE );
	}

	return ( *size <= *length );
}

// --

static S32
Search ( const uint8_t ** data, U32 * length, int * match )
{
	static const uint8_t base[] = { 0, 0, 0x10, 0, 0x80, 0, 0, 0x80, 0x5f, 0x9b, 0x34, 0xfb };

	const uint8_t * cursor;
	U32				size;
	U32				bytes;
	U32				index;
	uint16_t		uuid;

	if ( ( ! Sequence ( data, length, &size ) ) || ( ! size ) )
	{
		return ( FALSE );
	}

	*match = TRUE;
	*length -= size;
	cursor = *data;
	*data += size;

	while ( size )
	{
		switch ( *cursor++ )
		{
			case 0x19:
				bytes = 2;
				break;
			case 0x1a:
				bytes = 4;
				break;
			case 0x1c:
				bytes = 16;
				break;
			default:
				return ( FALSE );
		}

		if ( size <= bytes )
		{
			return ( FALSE );
		}

		uuid = BT_ReadBE16 ( cursor + ( bytes == 2 ? 0 : 2 ) );

		if ( ( bytes > 2 ) && ( cursor[0] || cursor[1] ) )
		{
			*match = FALSE;
		}

		if ( bytes == 16 )
		{
			for ( index = 0; index < sizeof ( base ); index++ )
			{
				if ( cursor[index + 4] != base[index] )
				{
					*match = FALSE;
				}
			}
		}

		if ( ( uuid != 0x1106 ) && ( uuid != 0x1002 ) && ( uuid != 0x0100 ) && ( uuid != 3 ) && ( uuid != 8 ) )
		{
			*match = FALSE;
		}

		cursor += bytes;
		size -= bytes + 1;
	}

	return ( TRUE );
}

// --

static S32
Attributes ( const uint8_t ** data, U32 * length, uint8_t * attributes, U32 * used, int match )
{
	const uint8_t * filters;
	const uint8_t * cursor;
	const uint8_t * record;
	U32				size;
	U32				remaining;
	U32				index;
	U32				byte;
	uint16_t		id;
	uint16_t		low;
	uint16_t		high;
	int				selected;

	if ( ! Sequence ( data, length, &size ) )
	{
		return ( FALSE );
	}

	filters = *data;
	*data += size;
	*length -= size;
	record = Record;

	for ( index = 0; index < sizeof ( AttributeSizes ); index++ )
	{
		id		  = BT_ReadBE16 ( record + 1 );
		cursor	  = filters;
		remaining = size;
		selected  = FALSE;

		while ( remaining )
		{
			if ( ( *cursor == 0x09 ) && ( remaining >= 3 ) )
			{
				low = high = BT_ReadBE16 ( cursor + 1 );

				cursor += 3;
				remaining -= 3;
			}
			else if ( ( *cursor == 0x0a ) && ( remaining >= 5 ) )
			{
				low	 = BT_ReadBE16 ( cursor + 1 );
				high = BT_ReadBE16 ( cursor + 3 );
				cursor += 5;
				remaining -= 5;
			}
			else
			{
				return ( FALSE );
			}

			selected |= ( id >= low ) && ( id <= high );
		}

		if ( ( selected ) && ( match ) )
		{
			for ( byte = 0; byte < AttributeSizes[index]; byte++ )
			{
				attributes[( *used )++] = record[byte];
			}
		}

		record += AttributeSizes[index];
	}

	return ( TRUE );
}

// --

enum BTHCIResult
BT2_ShareSDP ( struct BTL2CAPLink * link, uint16_t cid, const uint8_t * packet, uint16_t length, struct BTHCIError * error )
{
	uint8_t			response[sizeof ( Record ) + 16];
	uint8_t			attributes[sizeof ( Record ) + 4];
	const uint8_t * data;
	U32				remaining;
	U32				used;
	U32				limit;
	U32				offset;
	U32				count;
	U32				byte;
	int				match;

	if ( length < 5 )
	{
		return ( BT_HCI_INVALID_REPLY );
	}

	response[1] = packet[1];
	response[2] = packet[2];
	used		= 7;
	data		= packet + 5;
	remaining	= length - 5;
	match		= TRUE;

	if ( BT_ReadBE16 ( packet + 3 ) != remaining )
	{
		goto invalid;
	}

	switch ( packet[0] )
	{
		case 2: // Service Search.
			if ( ( ! Search ( &data, &remaining, &match ) ) || ( remaining != 3 ) || ( ! BT_ReadBE16 ( data ) ) || ( data[2] ) )
			{
				goto invalid;
			}

			response[0] = 3;
			response[5] = 0;
			response[6] = match;
			response[7] = 0;
			response[8] = match;
			used		= 9;

			if ( match )
			{
				response[used++] = SHARE_RECORD >> 24;
				response[used++] = ( SHARE_RECORD >> 16 ) & 255;
				response[used++] = 0;
				response[used++] = 0;
			}

			response[used++] = 0;
			break;

		case 4: // Service Attribute.
		case 6: // Service Search Attribute.
			if ( packet[0] == 4 )
			{
				if ( remaining < 4 )
				{
					goto invalid;
				}

				if ( ( BT_ReadBE16 ( data ) != ( SHARE_RECORD >> 16 ) ) || ( BT_ReadBE16 ( data + 2 ) ) )
				{
					response[0] = 1;
					response[5] = 0;
					response[6] = 2;
					break;
				}

				data += 4;
				remaining -= 4;
			}
			else if ( ! Search ( &data, &remaining, &match ) )
			{
				goto invalid;
			}

			if ( remaining < 2 )
			{
				goto invalid;
			}

			limit = BT_ReadBE16 ( data );
			data += 2;
			remaining -= 2;
			count = packet[0] == 4 || ! match ? 2 : 4;

			if ( ( limit < 7 ) || ( ! Attributes ( &data, &remaining, attributes, &count, match ) ) )
			{
				goto invalid;
			}

			attributes[0] = 0x35;
			attributes[1] = count - 2;

			if ( ( packet[0] == 6 ) && ( match ) )
			{
				attributes[2] = 0x35;
				attributes[3] = count - 4;
			}

			offset = 0;

			if ( ( remaining == 3 ) && ( data[0] == 2 ) )
			{
				offset = BT_ReadBE16 ( data + 1 );
			}
			else if ( ( remaining != 1 ) || ( data[0] ) )
			{
				goto invalid;
			}

			if ( offset >= count )
			{
				goto invalid;
			}

			if ( limit > link->PeerMTU - 10U )
			{
				limit = link->PeerMTU - 10U;
			}

			if ( limit > count - offset )
			{
				limit = count - offset;
			}

			response[0] = packet[0] + 1;
			response[5] = limit >> 8;
			response[6] = limit;

			for ( byte = 0; byte < limit; byte++ )
			{
				response[used++] = attributes[offset + byte];
			}

			offset += limit;
			response[used++] = offset < count ? 2 : 0;

			if ( offset < count )
			{
				response[used++] = offset >> 8;
				response[used++] = offset;
			}
			break;

		default:
			goto invalid;
	}

	goto reply;

invalid:
	response[0] = 1;
	response[5] = 0;
	response[6] = 3;
	used		= 7;

reply:
	response[3] = 0;
	response[4] = used - 5;

	return ( BT2_WriteL2CAP ( link, cid, response, used, error ) );
}

// --
