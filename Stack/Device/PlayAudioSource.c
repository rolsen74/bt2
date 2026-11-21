
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/dos.h>
#include "bt2_all.h"
#include "bt2_a2dp.h"

// --

struct BTAudioClock
{
	struct Bluetooth2Base *Base;
	struct BTDeadline Timer;
	struct TimeVal Start;
	struct TimeVal LastWrite;
	U32 Next;
	U32 PeakLag;
	U32 LatePackets;
	U32 Started;
};

// --

#define BT_AUDIO_PREROLL_US 40000U
#define BT_AUDIO_LAG_US 25000U
#define BT_AUDIO_RESYNC_US 120000U

// --

static int
PaceAudio( PTR context, uint32_t microseconds )
{
	struct BTAudioClock *clock;
	struct TimeVal now;

	U32 current;
	U32 wait;
	U32 lag;
	S32 result;

	clock = context;

	clock->Timer.Interface->GetUpTime( & now );

	current = now.Seconds * 1000000U + now.Microseconds;

	if ( ! microseconds )
	{
		clock->Next = current - BT_AUDIO_PREROLL_US;
		clock->Start = now;
		clock->LastWrite = now;
		clock->Started = TRUE;

		return ( BT_AUDIO_ON_TIME );
	}

	if ( microseconds != BT_A2DP_DRAIN_US )
	{
		clock->LastWrite = now;
	}

	clock->Next += microseconds;
	wait = clock->Next - current;

	if ( wait > 0x80000000U )
	{
		lag = current - clock->Next;

		if ( lag <= BT_AUDIO_PREROLL_US + BT_AUDIO_LAG_US )
		{
			return ( BT_AUDIO_ON_TIME );
		}

		lag -= BT_AUDIO_PREROLL_US;
		clock->LatePackets++;

		if ( lag > clock->PeakLag )
		{
			clock->PeakLag = lag;
		}

		if ( lag > BT_AUDIO_RESYNC_US )
		{
			// Avoid sending a long burst after a stalled link starts moving again.
			clock->Next = current - BT_AUDIO_PREROLL_US;
		}

		return ( BT_AUDIO_LATE );
	}

	if ( ! wait )
	{
		return ( BT_AUDIO_ON_TIME );
	}

	clock->Timer.Request->Request.io_Command = TR_ADDREQUEST;
	clock->Timer.Request->Time.Seconds = wait / 1000000U;
	clock->Timer.Request->Time.Microseconds = wait % 1000000U;

	result = clock->Base->Exec->DoIO( (PTR) clock->Timer.Request );

	return ( result ? BT_AUDIO_CLOCK_ERROR : BT_AUDIO_ON_TIME );
}

// --

static enum BTHCIResult
PlayAudioSource( struct Bluetooth2Base *base, const struct BTACLTransport *transport,
	struct BTAudioSource *source, struct BTHCIError *error )
{
	struct BTAudioClock clock;
	struct BTAudioPlayback *audio;
	enum BTHCIResult result;

	uint8_t key[BT_LINK_KEY_SIZE];
	U32 byte;

	audio = source->Request;

	if ( __atomic_load_n( & audio->Cancel, __ATOMIC_RELAXED ) )
	{
		return ( BT_RFCOMM_CANCELLED );
	}

	clock.Base = base;
	clock.Started = FALSE;
	clock.PeakLag = 0;
	clock.LatePackets = 0;
	source->Clock = & clock;
	source->Pace = PaceAudio;

	result = OpenDeadline( base, & clock.Timer, 0 );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	result = BT2_LoadBondKey( base, & transport->HCI, audio->Address, key, error );

	if ( ( result == BT_HCI_OK ) || ( result == BT_HCI_NO_SAVED_KEY ) )
	{
		result = BT2_PlayA2DP( transport, source, result == BT_HCI_OK ? key : NULL, error );
	}

	for( byte = 0; byte < sizeof( key ); byte++ )
	{
		((volatile uint8_t *) key)[byte] = 0;
	}

	if ( ( clock.Started ) && ( audio->FramesSent ) )
	{
		U32 elapsed;
		U32 expected;

		elapsed = ( clock.LastWrite.Seconds - clock.Start.Seconds ) * 1000U +
			clock.LastWrite.Microseconds / 1000U - clock.Start.Microseconds / 1000U;
		expected = ( audio->FramesSent / audio->Rate ) * 1000U +
			( audio->FramesSent % audio->Rate ) * 1000U / audio->Rate;

		base->Exec->DebugPrintF( "Bluetooth2 A2DP: %lu Hz, %lu sample frames (%lu ms), sent in %lu ms, %lu late packets, peak lag %lu ms.\n",
			(U32) audio->Rate, (U32) audio->FramesSent, expected, elapsed, clock.LatePackets, clock.PeakLag / 1000U );
	}

	CloseDeadline( base, & clock.Timer );

	return ( result );
}

// --
