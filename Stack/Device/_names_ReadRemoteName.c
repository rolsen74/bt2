
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
_names_ReadRemoteName( struct Bluetooth2NamesIFace *self __attribute__((unused)), const struct BTHCITransport *transport,
	struct BTRemoteName *remote, struct BTHCIError *error )
{
	return ( BT_ReadRemoteName( transport, remote, error ) );
}

// --
