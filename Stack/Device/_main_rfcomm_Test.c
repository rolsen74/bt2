
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_device.h"
#include "bt2_l2cap.h"

// --

enum BTHCIResult
_main_rfcomm_Test ( struct Bluetooth2IFace * self, const struct BTACLTransport * transport, struct BTRFCOMMTest * test,
					struct BTHCIError * error )
{
	return ( RunRFCOMMWithBond ( (PTR)self->Data.LibBase, transport, test, NULL, error ) );
}

// --
