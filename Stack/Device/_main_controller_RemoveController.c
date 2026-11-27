
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_device.h"

// --

void
_main_controller_RemoveController ( struct Bluetooth2IFace * self, U32 unit, struct MsgPort * port )
{
	struct Bluetooth2Base * base;
	struct BT2MasterRequest request;

	base			  = (PTR)self->Data.LibBase;
	request.Port	  = port;
	request.Unit	  = unit;
	request.Operation = BT2_MASTER_REMOVE;

	// The driver must be able to unregister even if reply-port allocation fails.
	if ( ! BT2_MasterCall ( base, &request ) )
	{
		BT2_RemoveController ( base, unit, port );
	}
}

// --
