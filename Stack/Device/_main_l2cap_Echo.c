
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
_main_l2cap_Echo ( struct Bluetooth2IFace * self __attribute__ ( ( unused ) ), const struct BTACLTransport * transport,
				   const uint8_t * address, struct BTHCIError * error )
{
	return ( BT_L2CAPEcho ( transport, address, error ) );
}

// --
