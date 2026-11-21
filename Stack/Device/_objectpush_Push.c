
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <dos/dostags.h>
#include <proto/dos.h>
#include "bt2_all.h"
#include "bt2_obex.h"
#include "bt2_sdp.h"

// --

static enum BTHCIResult
PushOpenedObject( struct Bluetooth2Base *base, const struct BTACLTransport *transport,
	struct BTFileSource *source, struct BTObjectPush *request, struct BTHCIError *error )
{
	struct BTOBEXPush push;
	struct ExamineData *info;
	enum BTHCIResult result;

	uint8_t key[BT_LINK_KEY_SIZE];
	uint64_t size;
	U32 byte;

	info = source->DOS->ExamineObjectTags( EX_FileHandleInput, source->Handle, TAG_END );

	if ( ! info )
	{
		request->FileError = source->DOS->IoErr();

		return ( BT_RFCOMM_FILE_ERROR );
	}

	size = info->FileSize;

	source->DOS->FreeDosObject( DOS_EXAMINEDATA, info );

	if ( size > UINT32_MAX )
	{
		return ( BT_OBEX_SIZE_LIMIT );
	}

	push.Context = source;
	push.Read = ReadSerialFile;
	push.Request = request;
	push.Size = size;

	push.Name = source->DOS->FilePart( request->Path );

	if ( ( ! push.Name ) || ( ! *push.Name ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	result = BT2_LoadBondKey( base, & transport->HCI, request->Peer.Address, key, error );

	if ( ( result == BT_HCI_OK ) || ( result == BT_HCI_NO_SAVED_KEY ) )
	{
		result = BT2_PushObject( transport, & push, result == BT_HCI_OK ? key : NULL, error );
	}

	for( byte = 0; byte < sizeof( key ); byte++ )
	{
		((volatile uint8_t *)key)[byte] = 0;
	}

	return ( result );
}

// --

static enum BTHCIResult
_objectpush_Push( struct Bluetooth2ObjectPushIFace *self, const struct BTACLTransport *transport,
	struct BTObjectPush *push, struct BTHCIError *error )
{
	struct Bluetooth2Base *base;
	struct BTFileSource source;
	struct Library *dosBase;
	enum BTHCIResult result;

	if ( ( ! push ) || ( ! push->Path ) || ( push->Peer.Channel > 30 ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	if ( __atomic_load_n( & push->Cancel, __ATOMIC_RELAXED ) )
	{
		return ( BT_RFCOMM_CANCELLED );
	}

	if ( ! push->Peer.Channel )
	{
		result = BT2_FindObjectPushChannel( transport, push->Peer.Address, & push->Peer.Channel, & push->Cancel, error );

		if ( result != BT_HCI_OK )
		{
			return ( result );
		}
	}

	base = (PTR) self->Data.LibBase;
	source.FileError = & push->FileError;
	result = BT_RFCOMM_FILE_ERROR;

	dosBase = base->Exec->OpenLibrary( "dos.library", 51 );

	if ( ! dosBase )
	{
		return ( result );
	}

	source.DOS = (PTR) base->Exec->GetInterface( dosBase, "main", 1, NULL );

	if ( source.DOS )
	{
		source.Handle = source.DOS->Open( push->Path, MODE_OLDFILE );

		if ( source.Handle )
		{
			result = PushOpenedObject( base, transport, & source, push, error );

			source.DOS->Close( source.Handle );
		}
		else
		{
			push->FileError = source.DOS->IoErr();
		}

		base->Exec->DropInterface( (PTR) source.DOS );
	}

	base->Exec->CloseLibrary( dosBase );

	return ( result );
}

// --
