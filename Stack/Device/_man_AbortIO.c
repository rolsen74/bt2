
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_device.h"

// --

void
_manager_AbortIO ( struct DeviceManagerInterface * self __attribute__ ( ( unused ) ),
				   struct IORequest *			   ioreq __attribute__ ( ( unused ) ) )
{
	// BeginIO completes every request synchronously.
}

// --
