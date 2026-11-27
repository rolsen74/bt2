
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
_main_hidinput_ConnectController ( struct Bluetooth2IFace * self, U32 unit, struct BTHIDInput * input,
								   struct BTHCIError * error )
{
	if ( ! ValidHIDInput ( input ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	return ( RequestHID ( (PTR)self->Data.LibBase, unit, &input->Monitor, error ) );
}

// --
