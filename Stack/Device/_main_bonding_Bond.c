
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_device.h"

// --

enum BTHCIResult
_main_bonding_Bond ( struct Bluetooth2IFace * self, const struct BTHCITransport * transport, struct BTBondRequest * bond,
					 struct BTHCIError * error )
{
	return ( BT2_Bond ( (PTR)self->Data.LibBase, transport, bond, error ) );
}

// --
