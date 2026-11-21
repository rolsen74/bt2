
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
_pcm_Play( struct Bluetooth2PCMIFace *self, const struct BTACLTransport *transport,
	struct BTAudioPCM *pcm, struct BTHCIError *error )
{
	struct BTAudioSource source;

	if ( ! ValidPCM( pcm ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	source.File = pcm->Context;
	source.Read = pcm->Read;
	source.Skip = NULL;
	source.Request = & pcm->Audio;
	source.Live = TRUE;
	source.Channels = 2;
	source.BytesLeft = 0;

	return ( PlayAudioSource( (PTR) self->Data.LibBase, transport, & source, error ) );
}

// --
