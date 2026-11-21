
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
_pairing_Pair( struct Bluetooth2PairingIFace *self __attribute__((unused)), const struct BTHCITransport *transport,
	const struct BTPairRequest *pair, struct BTHCIError *error )
{
	return ( BT_Pair( transport, pair, error ) );
}

// --
