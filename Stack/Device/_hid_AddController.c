
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_all.h"

// --

static S32
_hid_AddController( struct Bluetooth2HIDIFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location )
{
	return ( RegisterController( (PTR) self->Data.LibBase, port, location,
		LECapabilities( TRUE ) | BT2_CAP_A2DP | BT2_CAP_PCM | BT2_CAP_HID ) );
}

// --
