
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_device.h"
#include "bt2_l2cap.h"

// --

enum BTHCIResult
_main_rfcomm_TestController ( struct Bluetooth2IFace * self, U32 unit, struct BTRFCOMMTest * test, struct BTHCIError * error )
{
	struct BTControllerLocation location;
	struct BTControllerProbe	probe;

	if ( ! test )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	test->Stage			= BT_RFCOMM_STAGE_LINK;
	probe.Info			= NULL;
	probe.Error			= error;
	probe.Inquiry		= NULL;
	probe.Pair			= NULL;
	probe.Bond			= NULL;
	probe.RemoteName	= NULL;
	probe.Visibility	= NULL;
	probe.EchoAddress	= NULL;
	probe.Services		= NULL;
	probe.RFCOMM		= test;
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
