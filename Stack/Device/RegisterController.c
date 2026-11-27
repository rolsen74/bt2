
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
RegisterController ( struct Bluetooth2Base * base, struct MsgPort * port, const struct BTControllerLocation * location,
					 S32 capabilities )
{
	struct BT2MasterRequest request;

	request.Port		 = port;
	request.Location	 = location;
	request.Operation	 = BT2_MASTER_ADD;
	request.Capabilities = capabilities;

	if ( BT2_MasterCall ( base, &request ) )
	{
		return ( request.Result );
	}

	return ( BT2_AddController ( base, port, location, capabilities ) );
}

// --
