
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_share.h"

// --

static void
SetText ( struct BTShareSession * session, const char * text )
{
	while ( *text )
	{
		session->Text[session->TextLength++] = *text++;
	}
}

// --

S32
BT2_ShareOpenListing ( struct BTShareSession * session )
{
	struct DOSIFace * dos;

	dos = session->Server->DOS;

	session->Listing = dos->ObtainDirContextTags ( EX_LockInput, session->Server->Root, EX_DataFields,
												   EXF_TYPE | EXF_NAME | EXF_SIZE | EXF_REFCOUNT, TAG_END );

	if ( ! session->Listing )
	{
		return ( FALSE );
	}

	session->TextOffset = 0;
	session->TextLength = 0;

	SetText ( session, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\r\n"
					   "<!DOCTYPE folder-listing SYSTEM \"obex-folder-listing.dtd\">\r\n"
					   "<folder-listing version=\"1.0\">\r\n" );

	return ( TRUE );
}

// --

static S32
NextEntry ( struct BTShareSession * session )
{
	struct DOSIFace *	 dos;
	struct ExamineData * data;

	static const char close[] = "</folder-listing>\r\n";
	const char *	  name;
	U32				  size;
	U32				  count;
	char			  digits[10];

	dos					= session->Server->DOS;
	session->TextOffset = 0;
	session->TextLength = 0;

	while ( ( data = dos->ExamineDir ( session->Listing ) ) )
	{
		if ( ( EXD_IS_FILE ( data ) ) && ( ! EXD_IS_LINK ( data ) ) && ( data->RefCount <= 1 ) && ( data->FileSize >= 0 )
			 && ( data->FileSize <= UINT32_MAX ) && ( BT2_ShareValidName ( data->Name ) ) )
		{
			break;
		}
	}

	if ( ! data )
	{
		if ( dos->IoErr () != ERROR_NO_MORE_ENTRIES )
		{
			return ( FALSE );
		}

		dos->ReleaseDirContext ( session->Listing );

		session->Listing = NULL;

		SetText ( session, close );

		return ( TRUE );
	}

	SetText ( session, "  <file name=\"" );

	for ( name = data->Name; *name; name++ )
	{
		switch ( *name )
		{
			case '&':
				SetText ( session, "&amp;" );
				break;
			case '"':
				SetText ( session, "&quot;" );
				break;
			case '<':
				SetText ( session, "&lt;" );
				break;
			case '>':
				SetText ( session, "&gt;" );
				break;
			default:
				session->Text[session->TextLength++] = *name;
				break;
		}
	}

	SetText ( session, "\" size=\"" );

	size  = data->FileSize;
	count = 0;

	do
	{
		digits[count++] = '0' + size % 10;
		size /= 10;
	} while ( size );

	while ( count )
	{
		session->Text[session->TextLength++] = digits[--count];
	}

	SetText ( session, "\"/>\r\n" );

	return ( TRUE );
}

// --

S32
BT2_ShareReadListing ( struct BTShareSession * session, uint8_t * data, U32 capacity )
{
	U32 written;

	written = 0;

	while ( written < capacity )
	{
		while ( ( written < capacity ) && ( session->TextOffset < session->TextLength ) )
		{
			data[written++] = session->Text[session->TextOffset++];
		}

		if ( ( written == capacity ) || ( ! session->Listing ) )
		{
			break;
		}

		if ( ! NextEntry ( session ) )
		{
			return ( -1 );
		}
	}

	return ( written );
}

// --
