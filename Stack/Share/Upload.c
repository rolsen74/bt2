
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
TempName ( char * name, U32 value )
{
	static const char prefix[] = BT_SHARE_TEMP;
	static const char hex[]	   = "0123456789abcdef";

	U32 index;

	for ( index = 0; index < sizeof ( prefix ) - 1; index++ )
	{
		*name++ = prefix[index];
	}

	for ( index = 0; index < 8; index++ )
	{
		*name++ = hex[value & 15];
		value >>= 4;
	}

	*name = 0;
}

// --

S32
BT2_ShareOpenUpload ( struct BTShareSession * session, const char * name )
{
	struct DOSIFace * dos;
	struct DateStamp  stamp;

	BPTR previous;
	U32	 retry;
	U32	 seed;
	U32	 index;

	dos = session->Server->DOS;

	dos->DateStamp ( &stamp );

	seed = (U32)(uintptr_t)session ^ (U32)stamp.ds_Tick;

	previous = dos->SetCurrentDir ( session->Server->Root );

	for ( retry = 0; ( retry < 16 ) && ( ! session->Temp ); retry++ )
	{
		TempName ( session->TempName, seed + retry );

		session->Temp = dos->CreateDir ( session->TempName );

		if ( ( ! session->Temp ) && ( dos->IoErr () != ERROR_OBJECT_EXISTS ) )
		{
			break;
		}
	}

	if ( session->Temp )
	{
		dos->SetCurrentDir ( session->Temp );

		session->File = dos->Open ( "body", MODE_NEWFILE );
	}

	dos->SetCurrentDir ( previous );

	if ( ! session->File )
	{
		BT2_ShareClose ( session );

		return ( FALSE );
	}

	for ( index = 0; name[index]; index++ )
	{
		session->Name[index] = name[index];
	}

	session->Name[index] = 0;

	return ( TRUE );
}

// --

S32
BT2_ShareFinishUpload ( struct BTShareSession * session )
{
	struct DOSIFace * dos;

	char source[sizeof ( session->TempName ) + sizeof ( "/body" )];
	BPTR previous;
	S32	 result;
	U32	 index;

	dos	   = session->Server->DOS;
	result = FALSE;

	if ( dos->Close ( session->File ) )
	{
		previous = dos->SetCurrentDir ( session->Server->Root );

		for ( index = 0; session->TempName[index]; index++ )
		{
			source[index] = session->TempName[index];
		}

		source[index] = 0;

		if ( dos->AddPart ( source, "body", sizeof ( source ) ) )
		{
			// AmigaDOS Rename fails if the destination already exists.
			result = dos->Rename ( source, session->Name );
		}

		dos->SetCurrentDir ( previous );
	}

	session->File = 0;

	BT2_ShareClose ( session );

	return ( result );
}

// --

void
BT2_ShareClose ( struct BTShareSession * session )
{
	struct DOSIFace * dos;

	BPTR previous;

	dos = session->Server->DOS;

	if ( session->File )
	{
		dos->Close ( session->File );

		session->File = 0;
	}

	if ( session->Temp )
	{
		previous = dos->SetCurrentDir ( session->Temp );

		dos->Delete ( "body" );

		dos->SetCurrentDir ( session->Server->Root );

		dos->UnLock ( session->Temp );

		session->Temp = 0;

		dos->Delete ( session->TempName );

		dos->SetCurrentDir ( previous );
	}

	if ( session->Listing )
	{
		dos->ReleaseDirContext ( session->Listing );

		session->Listing = NULL;
	}

	session->HasLength = FALSE;
	session->Getting   = FALSE;
	session->Remaining = 0;
}

// --
