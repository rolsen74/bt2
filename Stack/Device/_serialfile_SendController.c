
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
	probe.HID = NULL;

	return ( BT2_RequestController( (PTR) self->Data.LibBase, unit, & location, & probe ) );
}

// --
