
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/dos.h>
#include "bt2_all.h"
#include "bt2_rfcomm.h"

// --

struct BTFileSource
{
	struct DOSIFace *DOS;
	int32_t *FileError;
	BPTR Handle;
};

// --

static int32_t
ReadSerialFile( void *context, uint8_t *data, uint32_t capacity )
{
	struct BTFileSource *source;

	S32 length;

	source = context;

	length = source->DOS->Read( source->Handle, data, capacity );

	if ( length < 0 )
	{
		*source->FileError = source->DOS->IoErr();
	}

	return ( length );
}

// --

static enum BTHCIResult
_serialfile_SendController( struct Bluetooth2SerialFileIFace *self, U32 unit,
	struct BTRFCOMMFile *file, struct BTHCIError *error )
{
	struct BTControllerLocation location;
	struct BTControllerProbe probe;

	if ( ( ! file ) || ( ! file->Path ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	file->Peer.Stage = BT_RFCOMM_STAGE_LINK;
	file->Sent = 0;
	file->FileError = 0;

	__atomic_store_n( & file->Cancel, 0, __ATOMIC_RELAXED );

	probe.Info = NULL;
	probe.Error = error;
	probe.Inquiry = NULL;
	probe.Pair = NULL;
	probe.Bond = NULL;
	probe.RemoteName = NULL;
	probe.Visibility = NULL;
	probe.EchoAddress = NULL;
	probe.Services = NULL;
	probe.RFCOMM = NULL;
	probe.Serial = NULL;
	probe.SerialFile = file;
	probe.SerialCapture = NULL;
	probe.ObjectPush = NULL;
	probe.LE = NULL;
	probe.Audio = NULL;
	probe.PCM = NULL;

	return ( BT2_RequestController( (PTR) self->Data.LibBase, unit, & location, & probe ) );
}

// --

static enum BTHCIResult
SendSerialFile( struct Bluetooth2Base *base, const struct BTACLTransport *transport,
	struct BTFileSource *source, struct BTRFCOMMFile *file, struct BTHCIError *error )
{
	struct BTRFCOMMStream stream;
	enum BTHCIResult result;

	uint8_t key[BT_LINK_KEY_SIZE];
	U32 byte;

	result = BT2_LoadBondKey( base, & transport->HCI, file->Peer.Address, key, error );

	if ( ( result == BT_HCI_OK ) || ( result == BT_HCI_NO_SAVED_KEY ) )
	{
		stream.Context = source;
		stream.Read = ReadSerialFile;
		stream.File = file;

		result = BT2_SendRFCOMMStream( transport, & stream, result == BT_HCI_OK ? key : NULL, error );
	}

	for( byte = 0; byte < sizeof( key ); byte++ )
	{
		((volatile uint8_t *)key)[byte] = 0;
	}

	return ( result );
}

// --

static enum BTHCIResult
_serialfile_Send( struct Bluetooth2SerialFileIFace *self, const struct BTACLTransport *transport,
	struct BTRFCOMMFile *file, struct BTHCIError *error )
{
	struct Bluetooth2Base *base;
	struct BTFileSource source;
	struct Library *dosBase;
	enum BTHCIResult result;

	if ( ( ! file ) || ( ! file->Path ) || ( ! file->Peer.Channel ) || ( file->Peer.Channel > 30 ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	if ( __atomic_load_n( & file->Cancel, __ATOMIC_RELAXED ) )
	{
		return ( BT_RFCOMM_CANCELLED );
	}

	base = (PTR) self->Data.LibBase;
	source.FileError = & file->FileError;
	result = BT_RFCOMM_FILE_ERROR;

	dosBase = base->Exec->OpenLibrary( "dos.library", 50 );

	if ( ! dosBase )
	{
		return ( result );
	}

	source.DOS = (PTR) base->Exec->GetInterface( dosBase, "main", 1, NULL );

	if ( source.DOS )
	{
		source.Handle = source.DOS->Open( file->Path, MODE_OLDFILE );

		if ( source.Handle )
		{
			result = SendSerialFile( base, transport, & source, file, error );

			source.DOS->Close( source.Handle );
		}
		else
		{
			file->FileError = source.DOS->IoErr();
		}

		base->Exec->DropInterface( (PTR) source.DOS );
	}

	base->Exec->CloseLibrary( dosBase );

	return ( result );
}

// --
