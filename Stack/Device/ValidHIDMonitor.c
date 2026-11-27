
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

S32
ValidHIDMonitor ( const struct BTHIDMonitor * monitor )
{
	return ( ( monitor ) && ( monitor->Reports ) && ( monitor->Capacity ) && ( monitor->Seconds )
			 && ( monitor->Seconds <= BT_HID_MAX_SECONDS ) );
}

// --
