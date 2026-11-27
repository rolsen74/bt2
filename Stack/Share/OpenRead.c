
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_share.h"

// --

S32
BT2_ShareValidName ( const char * name )
{
	U32		length;
	uint8_t ch;

	for ( length = 0; ( name[length] ) && ( length <= BT_SHARE_NAME ); length++ )
	{
		ch = name[length];

		if ( ( ch < 32 ) || ( ( ch >= 127 ) && ( ch < 160 ) ) || ( ch == ':' ) || ( ch == '/' ) || ( ch == '\\' )
			 || ( ch == '*' ) || ( ch == '?' ) )
		{
			return ( FALSE );
		}
	}

	// Keep internal files and dot paths outside the remote namespace.
	return ( length && ( length <= BT_SHARE_NAME ) && ( name[0] != '.' ) );
}

// --

static S32
SameName ( const char * requested, const char * actual )
{
	uint8_t first;
	uint8_t second;

	while ( *requested )
	{
		first  = *requested++;
		second = *actual++;

		if ( ( first >= 'A' ) && ( first <= 'Z' ) )
		{
			first += 'a' - 'A';
		}

		if ( ( second >= 'A' ) && ( second <= 'Z' ) )
		{
			second += 'a' - 'A';
		}

		if ( first != second )
		{
			return ( FALSE );
		}
	}

	return ( ! *actual );
}

// --

S32
BT2_ShareOpenRead ( struct BTShareSession * session, const char * name, U32 * size )
{
	struct DOSIFace *	 dos;
	struct ExamineData * data;

	BPTR previous;
	BPTR lock;
	BPTR parent;
	S32	 valid;

	dos = session->Server->DOS;

	previous = dos->SetCurrentDir ( session->Server->Root );

	lock = dos->Lock ( name, SHARED_LOCK );

	dos->SetCurrentDir ( previous );

	if ( ! lock )
	{
		return ( FALSE );
	}

	parent = dos->ParentDir ( lock );

	valid = parent && ( dos->SameLock ( parent, session->Server->Root ) == LOCK_SAME );

	if ( parent )
	{
		dos->UnLock ( parent );
	}

	if ( valid )
	{
		data = dos->ExamineObjectTags ( EX_LockInput, lock, EX_DataFields, EXF_TYPE | EXF_NAME | EXF_SIZE | EXF_REFCOUNT,
										TAG_END );

		if ( data )
		{
			valid = EXD_IS_FILE ( data ) && ! EXD_IS_LINK ( data ) && ( data->RefCount <= 1 ) && ( data->FileSize >= 0 )
					&& ( data->FileSize <= UINT32_MAX ) && ( data->Name ) && ( SameName ( name, data->Name ) );

			if ( valid )
			{
				*size = data->FileSize;
			}

			dos->FreeDosObject ( DOS_EXAMINEDATA, data );

			if ( valid )
			{
				session->File = dos->OpenFromLock ( lock );

				if ( session->File )
				{
					return ( TRUE );
				}
			}
		}
	}

	dos->UnLock ( lock );

	return ( FALSE );
}

// --
