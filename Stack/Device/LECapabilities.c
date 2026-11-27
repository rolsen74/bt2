
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_device.h"

// --

U32
LECapabilities ( U32 acl )
{
	U32 capabilities;

	capabilities = BT2_CAP_LE | BT2_CAP_INQUIRY | BT2_CAP_PAIRING | BT2_CAP_BONDING | BT2_CAP_NAMES | BT2_CAP_VISIBILITY;

	if ( acl )
	{
		capabilities |= BT2_CAP_L2CAP | BT2_CAP_SDP | BT2_CAP_RFCOMM | BT2_CAP_SERIAL | BT2_CAP_SERIALFILE
						| BT2_CAP_SERIALCAPTURE | BT2_CAP_OBJECTPUSH;
	}

	return ( capabilities );
}

// --
