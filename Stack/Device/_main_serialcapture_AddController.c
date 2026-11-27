
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
_main_serialcapture_AddController ( struct Bluetooth2IFace * self, struct MsgPort * port,
									const struct BTControllerLocation * location )
{
	return ( RegisterController ( (PTR)self->Data.LibBase, port, location,
								  BT2_CAP_INQUIRY | BT2_CAP_PAIRING | BT2_CAP_BONDING | BT2_CAP_NAMES | BT2_CAP_VISIBILITY
									  | BT2_CAP_L2CAP | BT2_CAP_SDP | BT2_CAP_RFCOMM | BT2_CAP_SERIAL | BT2_CAP_SERIALFILE
									  | BT2_CAP_SERIALCAPTURE ) );
}

// --
