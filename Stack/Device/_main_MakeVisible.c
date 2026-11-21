
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
_visibility_MakeVisible( struct Bluetooth2VisibilityIFace *self, U32 unit,
	const struct BTVisibility *visibility, struct BTHCIError *error )
{
	struct BTControllerLocation location;
	struct BTControllerProbe probe;

	if ( ( ! visibility ) || ( ! visibility->Seconds ) || ( visibility->Seconds > BT_VISIBLE_MAX_SECONDS ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	probe.Info = NULL;
	probe.Error = error;
	probe.Inquiry = NULL;
	probe.Pair = NULL;
	probe.Bond = NULL;
	probe.RemoteName = NULL;
	probe.Visibility = visibility;
	probe.EchoAddress = NULL;
	probe.Services = NULL;
	probe.RFCOMM = NULL;
	probe.Serial = NULL;
	probe.SerialFile = NULL;
	probe.SerialCapture = NULL;
	probe.ObjectPush = NULL;
	probe.LE = NULL;
	probe.Audio = NULL;
	probe.PCM = NULL;

	return ( BT2_RequestController( (PTR) self->Data.LibBase, unit, & location, & probe ) );
}

// --

static enum BTHCIResult
_visibility_RunVisible( struct Bluetooth2VisibilityIFace *self, const struct BTHCITransport *transport,
	const struct BTVisibility *visibility, struct BTHCIError *error )
{
	struct Bluetooth2Base *base;
	struct BTDeadline timer;
	enum BTHCIResult result;

	if ( ( ! visibility ) || ( ! visibility->Seconds ) || ( visibility->Seconds > BT_VISIBLE_MAX_SECONDS ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	base = (PTR) self->Data.LibBase;

	result = OpenDeadline( base, & timer, visibility->Seconds );

	if ( result == BT_HCI_OK )
	{
		result = BT2_RunVisible( transport, visibility, DeadlineExpired, & timer, error );

		CloseDeadline( base, & timer );
	}

	return ( result );
}

// --
