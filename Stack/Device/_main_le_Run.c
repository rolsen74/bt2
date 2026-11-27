
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_device.h"
#include "bt2_hci.h"

// --

enum BTHCIResult
_main_le_Run ( struct Bluetooth2IFace * self, const struct BTHCITransport * transport, struct BTLERequest * request,
			   struct BTHCIError * error )
{
	struct Bluetooth2Base * base;
	struct BTDeadline		timer;
	enum BTHCIResult		result;

	if ( ( ! request ) || ( ! request->Seconds ) || ( request->Seconds > BT_LE_MAX_SECONDS ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	base = (PTR)self->Data.LibBase;

	result = OpenDeadline ( base, &timer, request->Seconds );

	if ( result == BT_HCI_OK )
	{
		result = BT2_RunLE ( transport, request, DeadlineExpired, &timer, error );

		CloseDeadline ( base, &timer );
	}

	return ( result );
}

// --
