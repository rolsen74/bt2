
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
_sdp_Browse( struct Bluetooth2SDPIFace *self __attribute__((unused)),
	const struct BTACLTransport *transport, struct BTServiceQuery *query, struct BTHCIError *error )
{
	return ( BT_BrowseServices( transport, query, error ) );
}

// --
