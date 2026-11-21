
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_all.h"
#include "bt2_l2cap.h"

// --

static enum BTHCIResult
_serial_ExchangeController( struct Bluetooth2SerialIFace *self, U32 unit,
	struct BTRFCOMMTransfer *transfer, struct BTHCIError *error )
{
	struct BTControllerLocation location;
	struct BTControllerProbe probe;

	if ( ! transfer )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	transfer->Peer.Stage = BT_RFCOMM_STAGE_LINK;
	transfer->Sent = 0;
	transfer->ReplyLength = 0;
	transfer->ReplyTruncated = 0;
	probe.Info = NULL;
	probe.Error = error;
	probe.Inquiry = NULL;
	probe.Pair = NULL;
	probe.Bond = NULL;
	probe.RemoteName = NULL;
	probe.Visibility = NULL;
	probe.EchoAddress = NULL;
	probe.Services = NULL;
	probe.RFCOMM = NULL;
	probe.Serial = transfer;
	probe.SerialFile = NULL;
	probe.SerialCapture = NULL;
	probe.ObjectPush = NULL;
	probe.LE = NULL;
	probe.Audio = NULL;
	probe.PCM = NULL;

	return ( BT2_RequestController( (PTR) self->Data.LibBase, unit, & location, & probe ) );
}

// --

static enum BTHCIResult
_serial_Exchange( struct Bluetooth2SerialIFace *self, const struct BTACLTransport *transport,
	struct BTRFCOMMTransfer *transfer, struct BTHCIError *error )
{
	if ( ! transfer )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	return ( RunRFCOMMWithBond( (PTR) self->Data.LibBase, transport, & transfer->Peer, transfer, error ) );
}

// --
