
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
_l2cap_EchoController( struct Bluetooth2L2CAPIFace *self, U32 unit, const uint8_t *address, struct BTHCIError *error )
{
	struct BTControllerLocation location;
	struct BTControllerProbe probe;

	if ( ! address )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	probe.Info = NULL;
	probe.Error = error;
	probe.Inquiry = NULL;
	probe.Pair = NULL;
	probe.Bond = NULL;
	probe.RemoteName = NULL;
	probe.Visibility = NULL;
	probe.EchoAddress = address;
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
_l2cap_Echo( struct Bluetooth2L2CAPIFace *self __attribute__((unused)), const struct BTACLTransport *transport,
	const uint8_t *address, struct BTHCIError *error )
{
	return ( BT_L2CAPEcho( transport, address, error ) );
}

// --
