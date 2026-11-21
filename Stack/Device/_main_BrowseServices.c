
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
_sdp_BrowseController( struct Bluetooth2SDPIFace *self, U32 unit, struct BTServiceQuery *query, struct BTHCIError *error )
{
	struct BTControllerLocation location;
	struct BTControllerProbe probe;

	if ( ! query )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	query->Count = 0;
	query->Status = 0;
	query->Truncated = 0;
	probe.Info = NULL;
	probe.Error = error;
	probe.Inquiry = NULL;
	probe.Pair = NULL;
	probe.Bond = NULL;
	probe.RemoteName = NULL;
	probe.Visibility = NULL;
	probe.EchoAddress = NULL;
	probe.Services = query;
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
_sdp_Browse( struct Bluetooth2SDPIFace *self __attribute__((unused)),
	const struct BTACLTransport *transport, struct BTServiceQuery *query, struct BTHCIError *error )
{
	return ( BT_BrowseServices( transport, query, error ) );
}

// --
