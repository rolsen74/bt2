
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
_main_pairing_Pair ( struct Bluetooth2IFace * self __attribute__ ( ( unused ) ), const struct BTHCITransport * transport,
					 const struct BTPairRequest * pair, struct BTHCIError * error )
{
	return ( BT_Pair ( transport, pair, error ) );
}

// --
