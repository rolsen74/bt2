
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
_main_controller_ReadControllerInfo ( struct Bluetooth2IFace *		self __attribute__ ( ( unused ) ),
									  const struct BTHCITransport * transport, struct BTControllerInfo * info,
									  struct BTHCIError * error )
{
	return ( BT_ReadControllerInfo ( transport, info, error ) );
}

// --
