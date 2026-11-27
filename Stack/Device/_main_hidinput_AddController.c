
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_device.h"

// --

S32
_main_hidinput_AddController ( struct Bluetooth2IFace * self, struct MsgPort * port,
							   const struct BTControllerLocation * location )
{
	return ( RegisterController ( (PTR)self->Data.LibBase, port, location,
								  LECapabilities ( TRUE ) | BT2_CAP_A2DP | BT2_CAP_PCM | BT2_CAP_HID | BT2_CAP_HID_INPUT ) );
}

// --
