
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
_main_le_AddController ( struct Bluetooth2IFace * self, struct MsgPort * port, const struct BTControllerLocation * location,
						 U32 acl )
{
	return ( RegisterController ( (PTR)self->Data.LibBase, port, location, LECapabilities ( acl ) ) );
}

// --
