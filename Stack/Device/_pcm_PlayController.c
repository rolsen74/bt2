
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_all.h"
#include "bt2_a2dp.h"

// --

static enum BTHCIResult
_pcm_PlayController( struct Bluetooth2PCMIFace *self, U32 unit,
	struct BTAudioPCM *pcm, struct BTHCIError *error )
{
	struct BTControllerProbe probe;
	struct BTControllerLocation location;

	if ( ! ValidPCM( pcm ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	pcm->Audio.FileError = 0;
	pcm->Audio.FramesSent = 0;
	pcm->Audio.Signal = 0;
	pcm->Audio.Status = 0;

	__atomic_store_n( & pcm->Audio.Cancel, 0, __ATOMIC_RELAXED );

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
	probe.Serial = NULL;
	probe.SerialFile = NULL;
	probe.SerialCapture = NULL;
	probe.ObjectPush = NULL;
	probe.LE = NULL;
	probe.Audio = NULL;
	probe.PCM = pcm;
	probe.HID = NULL;

	return ( BT2_RequestController( (PTR) self->Data.LibBase, unit, & location, & probe ) );
}

// --
