
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
_names_ReadName( struct Bluetooth2NamesIFace *self, U32 unit,
	struct BTRemoteName *remote, struct BTHCIError *error )
{
	struct BTControllerLocation location;
	struct BTControllerProbe probe;

	if ( ! remote )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	remote->Name[0] = 0;
	probe.Info = NULL;
	probe.Error = error;
	probe.Inquiry = NULL;
	probe.Pair = NULL;
	probe.Bond = NULL;
	probe.RemoteName = remote;
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
_names_ReadRemoteName( struct Bluetooth2NamesIFace *self __attribute__((unused)), const struct BTHCITransport *transport,
	struct BTRemoteName *remote, struct BTHCIError *error )
{
	return ( BT_ReadRemoteName( transport, remote, error ) );
}

// --
