
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

static S32
ValidHIDInput( const struct BTHIDInput *input )
{
	return ( ( input ) && ( ! input->Monitor.Seconds ) && ( input->Start ) && ( input->Report ) && ( input->Stop ) );
}

// --
