
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_all.h"
#include "bt2_hid.h"

// --

static enum BTHCIResult
_hidinput_ConnectController( struct Bluetooth2HIDInputIFace *self, U32 unit,
	struct BTHIDInput *input, struct BTHCIError *error )
{
	if ( ! ValidHIDInput( input ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	return ( RequestHID( (PTR) self->Data.LibBase, unit, & input->Monitor, error ) );
}

// --
