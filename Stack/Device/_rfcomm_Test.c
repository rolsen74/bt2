
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_all.h"
#include "bt2_l2cap.h"

// --

static enum BTHCIResult
_rfcomm_Test( struct Bluetooth2RFCOMMIFace *self, const struct BTACLTransport *transport,
	struct BTRFCOMMTest *test, struct BTHCIError *error )
{
	return ( RunRFCOMMWithBond( (PTR) self->Data.LibBase, transport, test, NULL, error ) );
}

// --
