
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_all.h"

// --

static enum BTHCIResult
_main_DiscoverController( struct Bluetooth2IFace *self, U32 unit, struct BTControllerLocation *location,
	struct BTInquiryResult *inquiry, struct BTHCIError *error )
{
	struct BTControllerProbe probe;

	inquiry->Count = 0;
	inquiry->Truncated = 0;
	probe.Info = NULL;
	probe.Error = error;
	probe.Inquiry = inquiry;

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
	probe.LE = NULL;

	return ( BT2_RequestController( (PTR) self->Data.LibBase, unit, location, & probe ) );
}

// --
