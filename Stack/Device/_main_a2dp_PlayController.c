
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/dos.h>
#include "bt2_device.h"
#include "bt2_a2dp.h"

// --

enum BTHCIResult
_main_a2dp_PlayController ( struct Bluetooth2IFace * self, U32 unit, struct BTAudioPlayback * audio, struct BTHCIError * error )
{
	struct BTControllerLocation location;
	struct BTControllerProbe	probe;

	if ( ( ! audio ) || ( ! audio->Path ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	audio->FileError  = 0;
	audio->FramesSent = 0;
	audio->Rate		  = 0;
	audio->Signal	  = 0;
	audio->Status	  = 0;

	__atomic_store_n ( &audio->Cancel, 0, __ATOMIC_RELAXED );

	probe.Info			= NULL;
	probe.Error			= error;
	probe.Inquiry		= NULL;
	probe.Pair			= NULL;
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
	probe.Audio			= audio;
	probe.PCM			= NULL;
	probe.HID			= NULL;

	probe.Share = NULL;

	return ( BT2_RequestController ( (PTR)self->Data.LibBase, unit, &location, &probe ) );
}

// --
