
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_all.h"
#include "bt2_hid.h"

// --

static enum BTHCIResult
RequestHID( struct Bluetooth2Base *base, U32 unit, struct BTHIDMonitor *monitor, struct BTHCIError *error )
{
	struct BTControllerLocation location;
	struct BTControllerProbe probe;

	monitor->Count = 0;
	monitor->Handshake = 0xff;

	__atomic_store_n( & monitor->Cancel, 0, __ATOMIC_RELAXED );

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
	probe.LE = NULL;
	probe.Audio = NULL;
	probe.PCM = NULL;
	probe.HID = monitor;

	return ( BT2_RequestController( base, unit, & location, & probe ) );
}

// --
