
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_all.h"
#include "bt2_hci.h"

// --

static enum BTHCIResult
_le_RunController( struct Bluetooth2LEIFace *self, U32 unit,
	struct BTLERequest *request, struct BTHCIError *error )
{
	struct BTControllerLocation location;
	struct BTControllerProbe probe;

	if ( ! request )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	request->Count = 0;
	request->Truncated = 0;
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
	probe.ObjectPush = NULL;
	probe.LE = request;
	probe.Audio = NULL;
	probe.PCM = NULL;

	return ( BT2_RequestController( (PTR) self->Data.LibBase, unit, & location, & probe ) );
}

// --

static enum BTHCIResult
_le_Run( struct Bluetooth2LEIFace *self, const struct BTHCITransport *transport,
	struct BTLERequest *request, struct BTHCIError *error )
{
	struct Bluetooth2Base *base;
	struct BTDeadline timer;
	enum BTHCIResult result;

	if ( ( ! request ) || ( ! request->Seconds ) || ( request->Seconds > BT_LE_MAX_SECONDS ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	base = (PTR) self->Data.LibBase;

	result = OpenDeadline( base, & timer, request->Seconds );

	if ( result == BT_HCI_OK )
	{
		result = BT2_RunLE( transport, request, DeadlineExpired, & timer, error );

		CloseDeadline( base, & timer );
	}

	return ( result );
}

// --
