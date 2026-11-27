
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
_main_names_ReadRemoteName ( struct Bluetooth2IFace * self __attribute__ ( ( unused ) ), const struct BTHCITransport * transport,
							 struct BTRemoteName * remote, struct BTHCIError * error )
{
	return ( BT_ReadRemoteName ( transport, remote, error ) );
}

// --
