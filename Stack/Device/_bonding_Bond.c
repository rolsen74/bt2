
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_all.h"

// --

static enum BTHCIResult
_bonding_Bond( struct Bluetooth2BondingIFace *self, const struct BTHCITransport *transport,
	struct BTBondRequest *bond, struct BTHCIError *error )
{
	return ( BT2_Bond( (PTR) self->Data.LibBase, transport, bond, error ) );
}

// --
