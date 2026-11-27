
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_device.h"

// --

U32
_main_Release ( struct Interface * self )
{
	return ( --self->Data.RefCount );
}

// --
