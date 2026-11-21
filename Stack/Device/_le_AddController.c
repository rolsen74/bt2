
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
_le_AddController( struct Bluetooth2LEIFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location, U32 acl )
{
	return ( RegisterController( (PTR) self->Data.LibBase, port, location, LECapabilities( acl ) ) );
}

// --
