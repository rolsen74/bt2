
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_device.h"
#include "bt2_hid.h"

// --

enum BTHCIResult
_main_hid_MonitorController ( struct Bluetooth2IFace * self, U32 unit, struct BTHIDMonitor * monitor, struct BTHCIError * error )
{
	if ( ! ValidHIDMonitor ( monitor ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	return ( RequestHID ( (PTR)self->Data.LibBase, unit, monitor, error ) );
}

// --
