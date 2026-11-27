
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <dos/var.h>
#include <interfaces/dos.h>
#include "bt2_control.h"

// --

#define BOOT_SHARE_VAR	 "Bluetooth2/startup-share"
#define AUDIO_OUTPUT_VAR "Bluetooth2/audio-output"
#define SETTINGS_VERSION 1
#define SETTINGS_FLAGS	 ( BT_BOND_FLAGS | GVF_DONT_NULL_TERM )

// --

static struct DOSIFace *
OpenSettingsDOS ( struct Bluetooth2Base * base, struct Library ** library )
{
	*library = base->Exec->OpenLibrary ( "dos.library", 50 );

	if ( ! *library )
	{
		return ( NULL );
	}

	return ( (PTR)base->Exec->GetInterface ( *library, "main", 1, NULL ) );
}

// --

static S32
SaveSetting ( struct DOSIFace * dos, const char * name, const uint8_t * data, U32 length )
{
	uint8_t previous[BT_SHARE_PATH_SIZE + 9];
	S32		size;

	size = dos->GetVar ( name, (STRPTR)previous, sizeof ( previous ), SETTINGS_FLAGS );

	if ( ( size >= (S32)sizeof ( previous ) ) || ( size < 0 && dos->IoErr () != ERROR_OBJECT_NOT_FOUND ) )
	{
		return ( FALSE );
	}

	if ( dos->SetVar ( name, (CONST_STRPTR)data, length, BT_BOND_FLAGS | GVF_SAVE_VAR ) )
	{
		return ( TRUE );
	}

	// A failed persistent write can already have changed the live variable.
	if ( size >= 0 )
	{
		dos->SetVar ( name, (CONST_STRPTR)previous, size, BT_BOND_FLAGS | GVF_SAVE_VAR );
	}
	else
	{
		dos->DeleteVar ( name, BT_BOND_FLAGS | GVF_SAVE_VAR );
	}

	return ( FALSE );
}

// --

static S32
ValidPath ( const char * path )
{
	U32 index;
	S32 absolute;

	absolute = FALSE;

	for ( index = 0; ( index < BT_SHARE_PATH_SIZE ) && path[index]; index++ )
	{
		if ( ( index ) && ( path[index] == ':' ) )
		{
			absolute = TRUE;
		}
	}

	return ( absolute && ( index < BT_SHARE_PATH_SIZE ) );
}

// --
// The caller holds BondLock. 1 means a validated record, 0 means absent, -1 means invalid or unavailable.
S32
BT2_ReadStartupShare ( struct DOSIFace * dos, struct BT2StartupShare * settings )
{
	uint8_t data[BT_SHARE_PATH_SIZE + 9];
	S32		length;
	U32		index;

	settings->Enabled = FALSE;
	settings->Path[0] = 0;

	for ( index = 0; index < sizeof ( settings->Local ); index++ )
	{
		settings->Local[index] = 0;
	}

	length = dos->GetVar ( BOOT_SHARE_VAR, (STRPTR)data, sizeof ( data ), SETTINGS_FLAGS );

	if ( length < 0 )
	{
		return ( dos->IoErr () == ERROR_OBJECT_NOT_FOUND ? 0 : -1 );
	}

	if ( ( length < 9 ) || ( length >= (S32)sizeof ( data ) ) || ( data[0] != SETTINGS_VERSION ) || ( data[1] > 1 )
		 || ( data[length - 1] ) )
	{
		return ( -1 );
	}

	for ( index = 8; index < (U32)length - 1; index++ )
	{
		if ( ! data[index] )
		{
			return ( -1 );
		}
	}

	if ( ( data[1] ) && ( ! ValidPath ( (const char *)&data[8] ) ) )
	{
		return ( -1 );
	}

	settings->Enabled = data[1];

	for ( index = 0; index < 6; index++ )
	{
		settings->Local[index] = data[2 + index];
	}

	for ( index = 8; index < (U32)length; index++ )
	{
		settings->Path[index - 8] = data[index];
	}

	return ( 1 );
}

// --

void
BT2_ChangedPolicy ( struct Bluetooth2Base * base )
{
	base->Exec->ObtainSemaphore ( &base->ControllerLock );

	base->PolicyRevision++;

	base->Exec->ReleaseSemaphore ( &base->ControllerLock );
}

// --

enum BTHCIResult
BT2_StartupShareSettings ( struct Bluetooth2Base * base, struct BT2StartupShare * settings, U32 save )
{
	struct Library *  library;
	struct DOSIFace * dos;
	enum BTHCIResult  result;

	uint8_t data[BT_SHARE_PATH_SIZE + 8];
	uint8_t peer[6];
	U32		index;
	U32		length;

	if ( ( ! settings ) || ( save && ( ( settings->Enabled > 1 ) || ( settings->Enabled && ! ValidPath ( settings->Path ) ) ) ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	result = BT_HCI_BOND_IO_ERROR;

	dos = OpenSettingsDOS ( base, &library );

	if ( dos )
	{
		base->Exec->ObtainSemaphore ( &base->BondLock );

		if ( ! save )
		{
			result = BT2_ReadStartupShare ( dos, settings ) >= 0 ? BT_HCI_OK : BT_HCI_BOND_IO_ERROR;
		}
		else if ( ( settings->Enabled ) && ( BT2_ReadAutoPeer ( dos, settings->Local, peer ) ) )
		{
			result = BT_HCI_CONTROLLER_BUSY;
		}
		else
		{
			data[0] = SETTINGS_VERSION;
			data[1] = settings->Enabled;

			for ( index = 0; index < 6; index++ )
			{
				data[2 + index] = settings->Local[index];
			}

			length = 8;

			if ( settings->Enabled )
			{
				for ( index = 0; settings->Path[index]; index++ )
				{
					data[length++] = settings->Path[index];
				}
			}

			data[length++] = 0;

			if ( SaveSetting ( dos, BOOT_SHARE_VAR, data, length ) )
			{
				base->Exec->ObtainSemaphore ( &base->ControllerLock );

				base->ShareStartupArmed	 = FALSE;
				base->ShareStartupLoaded = TRUE;
				base->PolicyRevision++;

				base->Exec->ReleaseSemaphore ( &base->ControllerLock );

				result = BT_HCI_OK;
			}
			else
			{
				base->Exec->ObtainSemaphore ( &base->ControllerLock );

				base->ShareStartupArmed = FALSE;

				base->Exec->ReleaseSemaphore ( &base->ControllerLock );
			}
		}

		base->Exec->ReleaseSemaphore ( &base->BondLock );

		base->Exec->DropInterface ( (PTR)dos );
	}

	if ( library )
	{
		base->Exec->CloseLibrary ( library );
	}

	return ( result );
}

// --

enum BTHCIResult
BT2_AudioOutputSettings ( struct Bluetooth2Base * base, struct BT2AudioOutput * settings, U32 save )
{
	struct Library *  library;
	struct DOSIFace * dos;
	enum BTHCIResult  result;

	uint8_t data[15];
	S32		length;
	U32		index;

	if ( ( ! settings ) || ( save && ( settings->Selected > 1 ) ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	result = BT_HCI_BOND_IO_ERROR;

	dos = OpenSettingsDOS ( base, &library );

	if ( dos )
	{
		base->Exec->ObtainSemaphore ( &base->BondLock );

		if ( ( save ) && ( settings->Selected ) && ( ! BT2_HasSavedPeer ( dos, settings->Local, settings->Peer ) ) )
		{
			result = BT_HCI_NO_SAVED_KEY;
		}
		else if ( save )
		{
			data[0] = SETTINGS_VERSION;
			data[1] = settings->Selected;

			for ( index = 0; index < 6; index++ )
			{
				data[2 + index] = settings->Local[index];
				data[8 + index] = settings->Peer[index];
			}

			if ( SaveSetting ( dos, AUDIO_OUTPUT_VAR, data, 14 ) )
			{
				result = BT_HCI_OK;
			}
		}
		else
		{
			length = dos->GetVar ( AUDIO_OUTPUT_VAR, (STRPTR)data, sizeof ( data ), SETTINGS_FLAGS );

			if ( ( length == 14 ) && ( data[0] == SETTINGS_VERSION ) && ( data[1] <= 1 ) )
			{
				settings->Selected = data[1];

				for ( index = 0; index < 6; index++ )
				{
					settings->Local[index] = data[2 + index];
					settings->Peer[index]  = data[8 + index];
				}

				result = BT_HCI_OK;

				if ( ( settings->Selected ) && ( ! BT2_HasSavedPeer ( dos, settings->Local, settings->Peer ) ) )
				{
					settings->Selected = FALSE;
				}
			}
			else if ( ( length < 0 ) && ( dos->IoErr () == ERROR_OBJECT_NOT_FOUND ) )
			{
				settings->Selected = FALSE;
				result			   = BT_HCI_OK;
			}
		}

		base->Exec->ReleaseSemaphore ( &base->BondLock );

		base->Exec->DropInterface ( (PTR)dos );
	}

	if ( library )
	{
		base->Exec->CloseLibrary ( library );
	}

	return ( result );
}

// --
