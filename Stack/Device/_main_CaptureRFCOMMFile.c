
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

#define CAPTURE_PATH_SIZE 4096

struct BTFileSink
{
	struct DOSIFace *DOS;
	struct BTRFCOMMCapture *Request;
	BPTR Handle;
};

// --

static int32_t
WriteCaptureBytes( void *context, const uint8_t *data, uint32_t length )
{
	struct BTFileSink *sink;

	S32 written;

	sink = context;

	written = sink->DOS->Write( sink->Handle, data, length );

	if ( written <= 0 )
	{
		sink->Request->FileError = sink->DOS->IoErr();
	}

	return ( written );
}

// --

static BPTR
CreateCaptureDirectory( struct Bluetooth2Base *base, struct BTFileSink *sink, char *path, U32 *length )
{
	static const char label[] = ".bt2cap-";
	static const char hex[] = "0123456789abcdef";

	const char *destination;
	const char *leaf;
	U32 task;
	U32 prefix;
	U32 byte;
	U32 attempt;
	BPTR lock;

	destination = sink->Request->Path;

	leaf = sink->DOS->FilePart( destination );

	if ( ( ! leaf ) || ( ! *leaf ) )
	{
		sink->Request->FileError = ERROR_INVALID_COMPONENT_NAME;

		return ( 0 );
	}

	prefix = leaf - destination;

	if ( prefix + sizeof( label ) + 20 >= CAPTURE_PATH_SIZE )
	{
		sink->Request->FileError = ERROR_LINE_TOO_LONG;

		return ( 0 );
	}

	for( byte = 0; byte < prefix; byte++ )
	{
		path[byte] = destination[byte];
	}

	task = (U32)(uintptr_t) base->Exec->FindTask( NULL );

	for( byte = 0; byte < sizeof( label ) - 1; byte++ )
	{
		path[prefix++] = label[byte];
	}

	for( byte = 0; byte < 8; byte++ )
	{
		path[prefix++] = hex[( task >> ( 28 - 4 * byte ) ) & 15];
	}

	path[prefix++] = '-';
	path[prefix + 2] = 0;
	*length = prefix + 2;

	for( attempt = 0; attempt < 256; attempt++ )
	{
		path[prefix] = hex[attempt >> 4];
		path[prefix + 1] = hex[attempt & 15];

		lock = sink->DOS->CreateDir( path );

		if ( lock )
		{
			return ( lock );
		}

		sink->Request->FileError = sink->DOS->IoErr();

		if ( sink->Request->FileError != ERROR_OBJECT_EXISTS )
		{
			break;
		}
	}

	return ( 0 );
}

// --

static enum BTHCIResult
CaptureSerialFile( struct Bluetooth2Base *base, const struct BTACLTransport *transport,
	struct BTFileSink *file, struct BTHCIError *error )
{
	struct BTRFCOMMSink sink;
	enum BTHCIResult result;

	uint8_t key[BT_LINK_KEY_SIZE];
	U32 byte;

	result = BT2_LoadBondKey( base, & transport->HCI, file->Request->Peer.Address, key, error );

	if ( ( result == BT_HCI_OK ) || ( result == BT_HCI_NO_SAVED_KEY ) )
	{
		sink.Context = file;
		sink.Write = WriteCaptureBytes;
		sink.Capture = file->Request;

		result = BT2_CaptureRFCOMMStream( transport, & sink, result == BT_HCI_OK ? key : NULL, error );
	}

	for( byte = 0; byte < sizeof( key ); byte++ )
	{
		((volatile uint8_t *)key)[byte] = 0;
	}

	return ( result );
}

// --

static enum BTHCIResult
ReceiveFile( struct Bluetooth2Base *base, const struct BTACLTransport *transport,
	struct BTFileSink *sink, struct BTHCIError *error )
{
	enum BTHCIResult result;

	char *path;
	U32 dirLength;
	BPTR lock;
	S32 closed;

	lock = sink->DOS->Lock( sink->Request->Path, ACCESS_READ );

	if ( lock )
	{
		sink->DOS->UnLock( lock );

		sink->Request->FileError = ERROR_OBJECT_EXISTS;

		return ( BT_RFCOMM_FILE_ERROR );
	}

	sink->Request->FileError = sink->DOS->IoErr();

	if ( sink->Request->FileError != ERROR_OBJECT_NOT_FOUND )
	{
		return ( BT_RFCOMM_FILE_ERROR );
	}

	path = base->Exec->AllocVecTags( CAPTURE_PATH_SIZE, TAG_END );

	if ( ! path )
	{
		return ( BT_HCI_NO_MEMORY );
	}

	result = BT_RFCOMM_FILE_ERROR;

	lock = CreateCaptureDirectory( base, sink, path, & dirLength );

	if ( ! lock )
	{
		goto free_path;
	}

	// CreateDir obtains an exclusive lock; release it before opening a child.
	sink->DOS->UnLock( lock );

	sink->Request->FileError = 0;

	if ( ! sink->DOS->AddPart( path, "capture", CAPTURE_PATH_SIZE ) )
	{
		sink->Request->FileError = sink->DOS->IoErr();

		goto remove_directory;
	}

	sink->Handle = sink->DOS->Open( path, MODE_NEWFILE );

	if ( ! sink->Handle )
	{
		sink->Request->FileError = sink->DOS->IoErr();

		goto remove_directory;
	}

	result = CaptureSerialFile( base, transport, sink, error );

	closed = sink->DOS->Close( sink->Handle );

	if ( ( result == BT_HCI_OK ) && ( ! closed ) )
	{
		sink->Request->FileError = sink->DOS->IoErr();
		result = BT_RFCOMM_FILE_ERROR;
	}

	if ( ( result == BT_HCI_OK ) && ( ! sink->DOS->Rename( path, sink->Request->Path ) ) )
	{
		sink->Request->FileError = sink->DOS->IoErr();
		result = BT_RFCOMM_FILE_ERROR;
	}

	if ( result != BT_HCI_OK )
	{
		sink->DOS->Delete( path );
	}

remove_directory:
	path[dirLength] = 0;

	sink->DOS->Delete( path );

free_path:
	base->Exec->FreeVec( path );

	return ( result );
}

// --

static enum BTHCIResult
_serialcapture_ReceiveController( struct Bluetooth2SerialCaptureIFace *self, U32 unit,
	struct BTRFCOMMCapture *capture, struct BTHCIError *error )
{
	struct BTControllerLocation location;
	struct BTControllerProbe probe;

	if ( ( ! capture ) || ( ! capture->Path ) || ( ! capture->Expected ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	capture->Peer.Stage = BT_RFCOMM_STAGE_LINK;
	capture->Received = 0;
	capture->FileError = 0;

	__atomic_store_n( & capture->Cancel, 0, __ATOMIC_RELAXED );

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
	probe.SerialFile = NULL;
	probe.SerialCapture = capture;
	probe.ObjectPush = NULL;
	probe.LE = NULL;

	return ( BT2_RequestController( (PTR) self->Data.LibBase, unit, & location, & probe ) );
}

// --

static enum BTHCIResult
_serialcapture_Receive( struct Bluetooth2SerialCaptureIFace *self, const struct BTACLTransport *transport,
	struct BTRFCOMMCapture *capture, struct BTHCIError *error )
{
	struct Bluetooth2Base *base;
	struct BTFileSink sink;
	struct Library *dosBase;
	enum BTHCIResult result;

	if ( ( ! capture ) || ( ! capture->Path ) || ( ! capture->Expected ) ||
		( ! capture->Peer.Channel ) || ( capture->Peer.Channel > 30 ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	if ( __atomic_load_n( & capture->Cancel, __ATOMIC_RELAXED ) )
	{
		return ( BT_RFCOMM_CANCELLED );
	}

	base = (PTR) self->Data.LibBase;
	sink.Request = capture;
	result = BT_RFCOMM_FILE_ERROR;

	dosBase = base->Exec->OpenLibrary( "dos.library", 50 );

	if ( ! dosBase )
	{
		return ( result );
	}

	sink.DOS = (PTR) base->Exec->GetInterface( dosBase, "main", 1, NULL );

	if ( sink.DOS )
	{
		result = ReceiveFile( base, transport, & sink, error );

		base->Exec->DropInterface( (PTR) sink.DOS );
	}

	base->Exec->CloseLibrary( dosBase );

	return ( result );
}

// --
