
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <stddef.h>
#include "bt2_a2dp.h"

// --

#define RTP_SBC_HEADER 13
#define SBC_PACKET_FRAMES 8
#define SBC_FALLBACK_BITPOOL 18
#define SBC_LATE_STREAK 3

// --

void *
BT2_SBCMemset( void *memory, int byte, size_t count )
{
	volatile uint8_t *data;

	data = memory;

	while( count-- )
	{
		*data++ = byte;
	}

	return ( memory );
}

// --

static void
WriteBE32( uint8_t *data, uint32_t value )
{
	data[0] = value >> 24;
	data[1] = value >> 16;
	data[2] = value >> 8;
	data[3] = value;
}

// --

static int16_t
ReadPCM( const uint8_t *data )
{
	uint16_t value;

	value = BT_ReadLE16( data );

	return ( value < 32768 ? value : (int32_t) value - 65536 );
}

// --

static uint32_t
ReadSamples( struct BTAudioSource *source, int16_t *left, int16_t *right, int mono )
{
	uint8_t pcm[SBC_MAX_SAMPLES * 4];
	uint32_t length;
	uint32_t samples;
	uint32_t index;
	uint32_t stride;
	int32_t sample;

	stride = source->Channels * 2;
	length = SBC_MAX_SAMPLES * stride;

	if ( ( ! source->Live ) && ( length > source->BytesLeft ) )
	{
		length = source->BytesLeft;
	}

	if ( ! BT2_ReadAudioBytes( source, pcm, length ) )
	{
		return ( 0 );
	}

	if ( ! source->Live )
	{
		source->BytesLeft -= length;
	}

	samples = length / stride;

	for( index = 0; index < samples; index++ )
	{
		left[index] = ReadPCM( & pcm[index * stride] );

		right[index] = stride == 4 ? ReadPCM( & pcm[index * stride + 2] ) : left[index];

		if ( mono && ( stride == 4 ) )
		{
			sample = (int32_t) left[index] + right[index];
			left[index] = sample / 2;
		}
	}

	while( index < SBC_MAX_SAMPLES )
	{
		left[index] = 0;
		right[index++] = 0;
	}

	return ( samples );
}

// --

static uint32_t
PacketFrames( uint32_t mtu, uint32_t frameSize, uint32_t aclSize )
{
	uint32_t limit;
	uint32_t frames;
	uint32_t fragments;
	uint32_t best;
	uint32_t cost;

	limit = ( mtu - RTP_SBC_HEADER ) / frameSize;

	if ( limit > SBC_PACKET_FRAMES )
	{
		limit = SBC_PACKET_FRAMES;
	}

	if ( ! aclSize )
	{
		return ( limit );
	}

	best = 1;
	cost = ( BT_L2CAP_HEADER + RTP_SBC_HEADER + frameSize + aclSize - 1 ) / aclSize;

	for( frames = 2; frames <= limit; frames++ )
	{
		fragments = ( BT_L2CAP_HEADER + RTP_SBC_HEADER + frames * frameSize + aclSize - 1 ) / aclSize;

		if ( frames * cost >= best * fragments )
		{
			best = frames;
			cost = fragments;
		}
	}

	return ( best );
}

// --

enum BTHCIResult
BT2_StreamSBC( struct BTAVDTPSession *session, struct BTHCIError *error )
{
	struct BTAudioSource *source;
	enum BTHCIResult result;

	sbc_t encoder;
	uint8_t packet[BT_L2CAP_MTU];
	int16_t left[SBC_MAX_SAMPLES];
	int16_t right[SBC_MAX_SAMPLES];
	uint32_t mtu;
	uint32_t frameSize;
	uint32_t framesPerPacket;
	uint32_t count;
	uint32_t samples;
	uint32_t inputSamples;
	uint32_t timestamp;
	uint32_t interval;
	uint32_t remainder;
	uint32_t late;
	int pacing;
	int floor;
	uint16_t sequence;

	source = session->Source;
	mtu = session->Link->PeerMTU;

	if ( mtu > sizeof( packet ) )
	{
		mtu = sizeof( packet );
	}

	if ( mtu <= RTP_SBC_HEADER )
	{
		return ( BT_HCI_UNSUPPORTED );
	}

	frameSize = sbc_get_frame_size( & session->Codec );

	while( ( frameSize > mtu - RTP_SBC_HEADER ) && ( session->Codec.bitpool > session->MinimumBitpool ) )
	{
		session->Codec.bitpool--;

		frameSize = sbc_get_frame_size( & session->Codec );
	}

	if ( ( ! frameSize ) || ( frameSize > mtu - RTP_SBC_HEADER ) )
	{
		return ( BT_HCI_UNSUPPORTED );
	}

	framesPerPacket = PacketFrames( mtu, frameSize, session->Link->PacketSize );

	floor = session->MinimumBitpool;

	if ( floor < SBC_FALLBACK_BITPOOL )
	{
		floor = SBC_FALLBACK_BITPOOL;
	}

	sbc_reset( & encoder );

	late = 0;
	sequence = 0;
	timestamp = 0;
	remainder = 0;
	packet[0] = 0x80;
	packet[1] = 96;
	packet[8] = 'A';
	packet[9] = 'M';
	packet[10] = 'I';
	packet[11] = 'G';

	if ( ! source->Pace( source->Clock, 0 ) )
	{
		return ( BT_HCI_TIMER_ERROR );
	}

	while( ( source->Live ) || ( source->BytesLeft ) )
	{
		inputSamples = 0;

		for( count = 0; ( count < framesPerPacket ) && ( ( source->Live ) || ( source->BytesLeft ) ); count++ )
		{
			samples = ReadSamples( source, left, right, session->Codec.mode == SBC_MODE_MONO );

			if ( ! samples )
			{
				return ( BT2_LinkCancelled( session->Link ) ? BT_RFCOMM_CANCELLED : BT_RFCOMM_FILE_ERROR );
			}

			inputSamples += samples;

			if ( sbc_encode( & encoder, left, 1, right, 1, & session->Codec,
				& packet[RTP_SBC_HEADER + count * frameSize], frameSize ) )
			{
				return ( BT_HCI_UNSUPPORTED );
			}
		}

		packet[2] = sequence >> 8;
		packet[3] = sequence;
		packet[12] = count;

		WriteBE32( & packet[4], timestamp );

		result = BT2_WriteL2CAP( session->Link, session->MediaCID, packet, RTP_SBC_HEADER + count * frameSize, error );

		if ( result != BT_HCI_OK )
		{
			return ( result );
		}

		source->Request->FramesSent += inputSamples;
		sequence++;
		samples = count * SBC_MAX_SAMPLES;
		timestamp += samples;
		interval = samples * 1000000U + remainder;
		remainder = interval % source->Request->Rate;

		pacing = source->Pace( source->Clock, interval / source->Request->Rate );

		if ( pacing == BT_AUDIO_CLOCK_ERROR )
		{
			return ( BT_HCI_TIMER_ERROR );
		}

		late = pacing == BT_AUDIO_LATE ? late + 1 : 0;

		if ( ( late >= SBC_LATE_STREAK ) && ( session->Codec.bitpool > floor ) )
		{
			session->Codec.bitpool -= 4;

			if ( session->Codec.bitpool < floor )
			{
				session->Codec.bitpool = floor;
			}

			frameSize = sbc_get_frame_size( & session->Codec );

			framesPerPacket = PacketFrames( mtu, frameSize, session->Link->PacketSize );

			late = 0;
		}

		if ( BT2_LinkCancelled( session->Link ) )
		{
			return ( BT_RFCOMM_CANCELLED );
		}
	}

	// Give the sink time to drain its playback buffer before closing the stream.
	if ( ! source->Pace( source->Clock, BT_A2DP_DRAIN_US ) )
	{
		return ( BT_HCI_TIMER_ERROR );
	}

	return ( BT2_LinkCancelled( session->Link ) ? BT_RFCOMM_CANCELLED : BT_HCI_OK );
}

// --
