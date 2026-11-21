
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

static S32
ValidHIDMonitor( const struct BTHIDMonitor *monitor )
{
	return ( ( monitor ) && ( monitor->Reports ) && ( monitor->Capacity ) &&
		( monitor->Seconds ) && ( monitor->Seconds <= BT_HID_MAX_SECONDS ) );
}

// --
