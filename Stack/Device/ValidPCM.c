
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

static S32
ValidPCM( const struct BTAudioPCM *pcm )
{
	return ( ( pcm ) && ( pcm->Read ) && ( ( pcm->Audio.Rate == 44100 ) || ( pcm->Audio.Rate == 48000 ) ) );
}

// --
