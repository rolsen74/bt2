
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
_main_serial_Exchange ( struct Bluetooth2IFace * self, const struct BTACLTransport * transport,
						struct BTRFCOMMTransfer * transfer, struct BTHCIError * error )
{
	if ( ! transfer )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	return ( RunRFCOMMWithBond ( (PTR)self->Data.LibBase, transport, &transfer->Peer, transfer, error ) );
}

// --
