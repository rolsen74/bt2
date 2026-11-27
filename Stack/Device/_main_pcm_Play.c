
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_device.h"
#include "bt2_a2dp.h"

// --

enum BTHCIResult
_main_pcm_Play ( struct Bluetooth2IFace * self, const struct BTACLTransport * transport, struct BTAudioPCM * pcm,
				 struct BTHCIError * error )
{
	struct BTAudioSource source;

	if ( ! ValidPCM ( pcm ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	source.File		 = pcm->Context;
	source.Read		 = pcm->Read;
	source.Skip		 = NULL;
	source.Request	 = &pcm->Audio;
	source.Live		 = TRUE;
	source.Channels	 = 2;
	source.BytesLeft = 0;

	return ( PlayAudioSource ( (PTR)self->Data.LibBase, transport, &source, error ) );
}

// --
