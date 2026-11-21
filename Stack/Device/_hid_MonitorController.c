
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
_hid_MonitorController( struct Bluetooth2HIDIFace *self, U32 unit,
	struct BTHIDMonitor *monitor, struct BTHCIError *error )
{
	if ( ! ValidHIDMonitor( monitor ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	return ( RequestHID( (PTR) self->Data.LibBase, unit, monitor, error ) );
}

// --
