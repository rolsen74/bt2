
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
_l2cap_Echo( struct Bluetooth2L2CAPIFace *self __attribute__((unused)), const struct BTACLTransport *transport,
	const uint8_t *address, struct BTHCIError *error )
{
	return ( BT_L2CAPEcho( transport, address, error ) );
}

// --
