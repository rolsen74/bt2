
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

S32
ValidPCM ( const struct BTAudioPCM * pcm )
{
	return ( ( pcm ) && ( pcm->Read ) && ( ( pcm->Audio.Rate == 44100 ) || ( pcm->Audio.Rate == 48000 ) ) );
}

// --
