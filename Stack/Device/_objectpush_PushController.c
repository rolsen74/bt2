
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
_objectpush_PushController( struct Bluetooth2ObjectPushIFace *self, U32 unit,
	struct BTObjectPush *push, struct BTHCIError *error )
{
	struct BTControllerLocation location;
	struct BTControllerProbe probe;

	if ( ( ! push ) || ( ! push->Path ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	push->Peer.Stage = BT_RFCOMM_STAGE_LINK;
	push->Sent = 0;
	push->FileError = 0;
	push->Response = 0;

	__atomic_store_n( & push->Cancel, 0, __ATOMIC_RELAXED );

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
	probe.SerialCapture = NULL;
	probe.ObjectPush = push;
	probe.LE = NULL;
	probe.Audio = NULL;
	probe.PCM = NULL;
	probe.HID = NULL;

	return ( BT2_RequestController( (PTR) self->Data.LibBase, unit, & location, & probe ) );
}

// --
