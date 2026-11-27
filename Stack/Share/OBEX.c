
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

#define OBEX_CONTINUE	 0x90
#define OBEX_OK			 0xa0
#define OBEX_BAD_REQUEST 0xc0
#define OBEX_FORBIDDEN	 0xc3
#define OBEX_NOT_FOUND	 0xc4
#define OBEX_CONFLICT	 0xc9
#define OBEX_ERROR		 0xd0
#define OBEX_UNSUPPORTED 0xd1
#define SHARE_CONNECTION 1

static const uint8_t FileTransferTarget[]
	= { 0xf9, 0xec, 0x7b, 0xc4, 0x95, 0x3c, 0x11, 0xd2, 0x98, 0x4e, 0x52, 0x54, 0x00, 0xdc, 0x9e, 0x09 };

struct OBEXHeaders
{
	const uint8_t * Body;
	U32				BodyLength;
	U32				Length;
	char			Name[BT_SHARE_NAME + 1];
	uint8_t			HasName;
	uint8_t			HasLength;
	uint8_t			Listing;
	uint8_t			End;
	uint8_t			FTP;
	uint8_t			Connection;
};

// --

static U32
ReadBE32 ( const uint8_t * bytes )
{
	return ( ( (U32)BT_ReadBE16 ( bytes ) << 16 ) | BT_ReadBE16 ( bytes + 2 ) );
}

// --

static S32
SameBytes ( const uint8_t * value, U32 size, const uint8_t * expected, U32 wanted )
{
	U32 index;

	if ( size != wanted )
	{
		return ( FALSE );
	}

	for ( index = 0; index < size; index++ )
	{
		if ( value[index] != expected[index] )
		{
			return ( FALSE );
		}
	}

	return ( TRUE );
}

// --

static S32
DecodeHeaders ( struct OBEXHeaders * headers, const uint8_t * data, U32 remaining )
{
	static const uint8_t listingType[] = "x-obex/folder-listing";

	const uint8_t * value;
	U32				length;
	U32				index;
	uint8_t			id;

	while ( remaining )
	{
		id = data[0];

		switch ( id >> 6 )
		{
			case 0:
			case 1:
				if ( remaining < 3 )
				{
					return ( FALSE );
				}

				length = BT_ReadBE16 ( data + 1 );

				if ( ( length < 3 ) || ( length > remaining ) )
				{
					return ( FALSE );
				}

				value = data + 3;
				data += length;
				remaining -= length;
				length -= 3;
				break;

			default:
				length = id < 0xc0 ? 1 : 4;

				if ( remaining <= length )
				{
					return ( FALSE );
				}

				value = data + 1;
				data += length + 1;
				remaining -= length + 1;
				break;
		}

		switch ( id )
		{
			case 0x01: // Name (UTF-16BE).
				if ( ( headers->HasName ) || ( length & 1 ) || ( length > 2 * sizeof ( headers->Name ) )
					 || ( ( length ) && ( BT_ReadBE16 ( value + length - 2 ) ) ) )
				{
					return ( FALSE );
				}

				for ( index = 0; ( index + 2 ) < length; index += 2 )
				{
					if ( ( value[index] ) || ( ! value[index + 1] ) )
					{
						return ( FALSE );
					}

					headers->Name[index / 2] = value[index + 1];
				}

				headers->Name[index / 2] = 0;
				headers->HasName		 = TRUE;
				break;

			case 0x42: // Type.
				headers->Listing = SameBytes ( value, length, listingType, sizeof ( listingType ) );
				break;

			case 0x46: // Target.
				headers->FTP = SameBytes ( value, length, FileTransferTarget, sizeof ( FileTransferTarget ) );
				break;

			case 0x48: // Body.
			case 0x49: // End of Body.
				if ( headers->Body )
				{
					return ( FALSE );
				}

				headers->Body		= value;
				headers->BodyLength = length;
				headers->End		= id == 0x49;
				break;

			case 0xc3: // Length.
				if ( headers->HasLength )
				{
					return ( FALSE );
				}

				headers->HasLength = TRUE;
				headers->Length	   = ReadBE32 ( value );
				break;

			case 0xcb: // Connection ID.
				if ( ( headers->Connection ) || ( ReadBE32 ( value ) != SHARE_CONNECTION ) )
				{
					return ( FALSE );
				}

				headers->Connection = TRUE;
				break;
		}
	}

	return ( TRUE );
}

// --

static enum BTHCIResult
Reply ( struct BTShareSession * session, uint8_t code, struct BTHCIError * error )
{
	uint8_t packet[] = { 0, 0, 3 };

	packet[0] = code;

	return ( BT2_ShareSend ( session, packet, sizeof ( packet ), error ) );
}

// --

static enum BTHCIResult
Connect ( struct BTShareSession * session, const uint8_t * input, const struct OBEXHeaders * headers, struct BTHCIError * error )
{
	uint8_t packet[31];
	U32		byte;

	session->Limit = BT_ReadBE16 ( input + 5 );

	if ( ( session->OBEXConnected ) || ( ! headers->FTP ) || ( session->Limit < 255 ) || ( input[4] ) )
	{
		return ( Reply ( session, OBEX_BAD_REQUEST, error ) );
	}

	if ( session->Limit > BT_SHARE_PACKET )
	{
		session->Limit = BT_SHARE_PACKET;
	}

	packet[0]  = OBEX_OK;
	packet[1]  = 0;
	packet[2]  = sizeof ( packet );
	packet[3]  = 0x10;
	packet[4]  = 0;
	packet[5]  = BT_SHARE_PACKET >> 8;
	packet[6]  = BT_SHARE_PACKET & 255;
	packet[7]  = 0xcb;
	packet[8]  = 0;
	packet[9]  = 0;
	packet[10] = 0;
	packet[11] = SHARE_CONNECTION;
	packet[12] = 0x4a; // Who.
	packet[13] = 0;
	packet[14] = 3 + sizeof ( FileTransferTarget );

	for ( byte = 0; byte < sizeof ( FileTransferTarget ); byte++ )
	{
		packet[15 + byte] = FileTransferTarget[byte];
	}

	session->OBEXConnected = TRUE;

	return ( BT2_ShareSend ( session, packet, sizeof ( packet ), error ) );
}

// --

static enum BTHCIResult
Get ( struct BTShareSession * session, const struct OBEXHeaders * headers, struct BTHCIError * error )
{
	uint8_t packet[BT_SHARE_PACKET];
	U32		offset;
	U32		count;
	U32		size;
	S32		actual;
	S32		done;

	offset = 3;

	if ( ! session->Getting )
	{
		if ( headers->Listing )
		{
			if ( ( headers->Name[0] ) || ( ! BT2_ShareOpenListing ( session ) ) )
			{
				return ( Reply ( session, OBEX_NOT_FOUND, error ) );
			}
		}
		else
		{
			if ( ( ! BT2_ShareValidName ( headers->Name ) ) || ( ! BT2_ShareOpenRead ( session, headers->Name, &size ) ) )
			{
				return ( Reply ( session, OBEX_NOT_FOUND, error ) );
			}

			session->Remaining = size;
			packet[offset++]   = 0xc3;
			packet[offset++]   = size >> 24;
			packet[offset++]   = size >> 16;
			packet[offset++]   = size >> 8;
			packet[offset++]   = size;
		}

		session->Getting = TRUE;
	}
	else if ( ( headers->HasName ) || ( headers->Listing ) )
	{
		BT2_ShareClose ( session );

		return ( Reply ( session, OBEX_BAD_REQUEST, error ) );
	}

	count = session->Limit - offset - 3;

	if ( session->File )
	{
		if ( count > session->Remaining )
		{
			count = session->Remaining;
		}

		actual = count ? session->Server->DOS->Read ( session->File, packet + offset + 3, count ) : 0;

		if ( actual != (S32)count )
		{
			goto fail;
		}

		session->Remaining -= count;
		done = ! session->Remaining;
	}
	else
	{
		actual = BT2_ShareReadListing ( session, packet + offset + 3, count );

		if ( actual < 0 )
		{
			goto fail;
		}

		count = actual;
		done  = ( ! session->Listing ) && ( session->TextOffset == session->TextLength );
	}

	packet[0]		 = done ? OBEX_OK : OBEX_CONTINUE;
	packet[1]		 = ( offset + count + 3 ) >> 8;
	packet[2]		 = offset + count + 3;
	packet[offset++] = done ? 0x49 : 0x48;
	packet[offset++] = ( count + 3 ) >> 8;
	packet[offset++] = count + 3;

	if ( done )
	{
		BT2_ShareClose ( session );
	}

	return ( BT2_ShareSend ( session, packet, offset + count, error ) );

fail:
	BT2_ShareClose ( session );

	return ( Reply ( session, OBEX_ERROR, error ) );
}

// --

static enum BTHCIResult
Put ( struct BTShareSession * session, const struct OBEXHeaders * headers, S32 final, struct BTHCIError * error )
{
	struct DOSIFace * dos;

	U32		bytes;
	uint8_t result;

	dos	   = session->Server->DOS;
	bytes  = headers->BodyLength;
	result = OBEX_BAD_REQUEST;

	if ( ( final != headers->End ) || ( headers->Listing ) )
	{
		goto fail;
	}

	if ( ! session->Temp )
	{
		if ( ! BT2_ShareValidName ( headers->Name ) )
		{
			goto fail;
		}

		if ( ! BT2_ShareOpenUpload ( session, headers->Name ) )
		{
			result = OBEX_ERROR;

			goto fail;
		}

		session->HasLength = headers->HasLength;
		session->Remaining = headers->Length;
	}
	else if ( ( headers->HasName ) || ( headers->HasLength ) )
	{
		goto fail;
	}

	if ( ( session->HasLength ) && ( bytes > session->Remaining ) )
	{
		goto fail;
	}

	if ( ( bytes ) && ( dos->Write ( session->File, (PTR)headers->Body, bytes ) != (S32)bytes ) )
	{
		result = OBEX_ERROR;

		goto fail;
	}

	if ( session->HasLength )
	{
		session->Remaining -= bytes;
	}

	if ( ! final )
	{
		return ( Reply ( session, OBEX_CONTINUE, error ) );
	}

	if ( ( session->HasLength ) && ( session->Remaining ) )
	{
		goto fail;
	}

	result = BT2_ShareFinishUpload ( session ) ? OBEX_OK : OBEX_CONFLICT;

	return ( Reply ( session, result, error ) );

fail:
	BT2_ShareClose ( session );

	return ( Reply ( session, result, error ) );
}

// --

enum BTHCIResult
BT2_ShareOBEX ( struct BTShareSession * session, const uint8_t * input, uint16_t length, struct BTHCIError * error )
{
	static const struct OBEXHeaders empty = { 0 };
	struct OBEXHeaders				headers;

	U32		offset;
	uint8_t access;
	uint8_t operation;

	operation = input[0];
	offset	  = operation == 0x80 ? 7 : operation == 0x85 ? 5 : 3;

	session->Server->Base->Exec->CopyMem ( &empty, &headers, sizeof ( headers ) );

	if ( ( length < offset ) || ( ! DecodeHeaders ( &headers, &input[offset], length - offset ) ) )
	{
		BT2_ShareClose ( session );

		return ( Reply ( session, OBEX_BAD_REQUEST, error ) );
	}

	access = session->Access & BT2_ShareAccess ( session, NULL );

	if ( ! access )
	{
		BT2_ShareClose ( session );

		return ( Reply ( session, OBEX_FORBIDDEN, error ) );
	}

	if ( operation == 0x80 )
	{
		return ( Connect ( session, input, &headers, error ) );
	}

	if ( ! session->OBEXConnected )
	{
		return ( Reply ( session, OBEX_BAD_REQUEST, error ) );
	}

	switch ( operation )
	{
		case 0x81: // Disconnect.
			session->OBEXConnected = FALSE;
			/* fall through */
		case 0xff: // Abort.
			BT2_ShareClose ( session );

			return ( Reply ( session, OBEX_OK, error ) );

		case 0x85: // SetPath to the shared root.
			BT2_ShareClose ( session );

			return ( Reply ( session, ( input[4] || ( input[3] & ~3 ) || headers.Name[0] ) ? OBEX_NOT_FOUND : OBEX_OK, error ) );

		case 0x83: // Final Get and subsequent body requests.
			if ( ( access & BT_SHARE_READ ) && ( ! session->Temp ) && ( ! headers.Body ) )
			{
				return ( Get ( session, &headers, error ) );
			}
			break;

		case 0x02:
		case 0x82:
			if ( ( access & BT_SHARE_WRITE ) && ( ! session->Getting ) )
			{
				return ( Put ( session, &headers, operation == 0x82, error ) );
			}
			break;

		default:
			return ( Reply ( session, OBEX_UNSUPPORTED, error ) );
	}

	BT2_ShareClose ( session );

	return ( Reply ( session, OBEX_FORBIDDEN, error ) );
}

// --
