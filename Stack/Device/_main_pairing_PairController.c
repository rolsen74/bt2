
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_device.h"

// --

enum BTHCIResult
_main_pairing_PairController ( struct Bluetooth2IFace * self, U32 unit, const struct BTPairRequest * pair,
							   struct BTHCIError * error )
{
	struct BTControllerLocation location;
	struct BTControllerProbe	probe;

	if ( ! pair )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	probe.Info	  = NULL;
	probe.Error	  = error;
	probe.Inquiry = NULL;
	probe.Pair	  = pair;

	probe.Bond			= NULL;
	probe.RemoteName	= NULL;
	probe.Visibility	= NULL;
	probe.EchoAddress	= NULL;
	probe.Services		= NULL;
	probe.RFCOMM		= NULL;
	probe.Serial		= NULL;
	probe.SerialFile	= NULL;
	probe.SerialCapture = NULL;
	probe.ObjectPush	= NULL;
	probe.LE			= NULL;
	probe.Audio			= NULL;
	probe.PCM			= NULL;
	probe.HID			= NULL;

	probe.Share = NULL;

	return ( BT2_RequestController ( (PTR)self->Data.LibBase, unit, &location, &probe ) );
}

// --
