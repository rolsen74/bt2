
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/dos.h>
#include "bt2_device.h"
#include "bt2_a2dp.h"

// --

static int
SkipWaveBytes ( PTR context, uint32_t length )
{
	struct BTFileSource * source;
	LONG				  success;

	source = context;

	success = source->DOS->ChangeFilePosition ( source->Handle, length, OFFSET_CURRENT );

	if ( ! success )
	{
		*source->FileError = source->DOS->IoErr ();
	}

	return ( success );
}

// --

static enum BTHCIResult
PlayAudioFile ( struct Bluetooth2Base * base, const struct BTACLTransport * transport, struct BTFileSource * file,
				struct BTAudioPlayback * audio, struct BTHCIError * error )
{
	struct BTAudioSource source;
	enum BTHCIResult	 result;

	source.File	   = file;
	source.Read	   = ReadSerialFile;
	source.Skip	   = SkipWaveBytes;
	source.Request = audio;

	result = BT2_ReadWave ( &source );

	if ( __atomic_load_n ( &audio->Cancel, __ATOMIC_RELAXED ) )
	{
		return ( BT_RFCOMM_CANCELLED );
	}

	if ( result == BT_HCI_OK )
	{
		result = PlayAudioSource ( base, transport, &source, error );
	}

	return ( result );
}

// --

enum BTHCIResult
_main_a2dp_Play ( struct Bluetooth2IFace * self, const struct BTACLTransport * transport, struct BTAudioPlayback * audio,
				  struct BTHCIError * error )
{
	struct Bluetooth2Base * base;
	struct BTFileSource		file;
	struct Library *		dosBase;
	enum BTHCIResult		result;

	if ( ( ! audio ) || ( ! audio->Path ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	base		   = (PTR)self->Data.LibBase;
	file.FileError = &audio->FileError;
	result		   = BT_RFCOMM_FILE_ERROR;

	dosBase = base->Exec->OpenLibrary ( "dos.library", 51 );

	if ( ! dosBase )
	{
		return ( result );
	}

	file.DOS = (PTR)base->Exec->GetInterface ( dosBase, "main", 1, NULL );

	if ( file.DOS )
	{
		file.Handle = file.DOS->Open ( audio->Path, MODE_OLDFILE );

		if ( file.Handle )
		{
			result = PlayAudioFile ( base, transport, &file, audio, error );

			file.DOS->Close ( file.Handle );
		}
		else
		{
			audio->FileError = file.DOS->IoErr ();
		}

		base->Exec->DropInterface ( (PTR)file.DOS );
	}

	base->Exec->CloseLibrary ( dosBase );

	return ( result );
}

// --
