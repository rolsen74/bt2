
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
_main_Inquiry( struct Bluetooth2IFace *self __attribute__((unused)), const struct BTHCITransport *transport,
	struct BTInquiryResult *inquiry, struct BTHCIError *error )
{
	return ( BT_Inquiry( transport, inquiry, error ) );
}

// --
