
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
ValidHIDInput ( const struct BTHIDInput * input )
{
	return ( ( input ) && ( ! input->Monitor.Seconds ) && ( input->Start ) && ( input->Report ) && ( input->Stop ) );
}

// --
