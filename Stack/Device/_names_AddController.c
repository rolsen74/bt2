
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_all.h"

// --

static S32
_names_AddController( struct Bluetooth2NamesIFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location )
{
	return ( RegisterController( (PTR) self->Data.LibBase, port, location,
		BT2_CAP_INQUIRY | BT2_CAP_PAIRING | BT2_CAP_BONDING | BT2_CAP_NAMES ) );
}

// --
