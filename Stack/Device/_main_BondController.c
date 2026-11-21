
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
_bonding_BondController( struct Bluetooth2BondingIFace *self, U32 unit,
	struct BTBondRequest *bond, struct BTHCIError *error )
{
	struct BTControllerLocation location;
	struct BTControllerProbe probe;

	if ( ! bond )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	bond->SentSavedKey = 0;
	probe.Info = NULL;
	probe.Error = error;
	probe.Inquiry = NULL;
	probe.Pair = NULL;
	probe.Bond = bond;
	probe.RemoteName = NULL;
	probe.Visibility = NULL;
	probe.EchoAddress = NULL;
	probe.Services = NULL;
	probe.RFCOMM = NULL;
	probe.Serial = NULL;
	probe.SerialFile = NULL;
	probe.SerialCapture = NULL;
	probe.ObjectPush = NULL;
	probe.LE = NULL;

	return ( BT2_RequestController( (PTR) self->Data.LibBase, unit, & location, & probe ) );
}

// --

static enum BTHCIResult
_bonding_Bond( struct Bluetooth2BondingIFace *self, const struct BTHCITransport *transport,
	struct BTBondRequest *bond, struct BTHCIError *error )
{
	return ( BT2_Bond( (PTR) self->Data.LibBase, transport, bond, error ) );
}

// --
