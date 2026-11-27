
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
_main_serial_ExchangeController ( struct Bluetooth2IFace * self, U32 unit, struct BTRFCOMMTransfer * transfer,
								  struct BTHCIError * error )
{
	struct BTControllerLocation location;
	struct BTControllerProbe	probe;

	if ( ! transfer )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	transfer->Peer.Stage	 = BT_RFCOMM_STAGE_LINK;
	transfer->Sent			 = 0;
	transfer->ReplyLength	 = 0;
	transfer->ReplyTruncated = 0;
	probe.Info				 = NULL;
	probe.Error				 = error;
	probe.Inquiry			 = NULL;
	probe.Pair				 = NULL;
	probe.Bond				 = NULL;
	probe.RemoteName		 = NULL;
	probe.Visibility		 = NULL;
	probe.EchoAddress		 = NULL;
	probe.Services			 = NULL;
	probe.RFCOMM			 = NULL;
	probe.Serial			 = transfer;
	probe.SerialFile		 = NULL;
	probe.SerialCapture		 = NULL;
	probe.ObjectPush		 = NULL;
	probe.LE				 = NULL;
	probe.Audio				 = NULL;
	probe.PCM				 = NULL;
	probe.HID				 = NULL;

	probe.Share = NULL;

	return ( BT2_RequestController ( (PTR)self->Data.LibBase, unit, &location, &probe ) );
}

// --
