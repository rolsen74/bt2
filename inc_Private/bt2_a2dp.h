#ifndef BT2_A2DP_H
#define BT2_A2DP_H

#include "bt2_l2cap.h"
#include "sbc.h"

#define BT_AVDTP_PSM 0x0019
#define BT_AVDTP_SIGNAL_CID 0x0040
#define BT_AVDTP_MEDIA_CID 0x0041
#define BT_A2DP_DRAIN_US 200000

enum BTAudioPacing
{
	BT_AUDIO_CLOCK_ERROR,
	BT_AUDIO_ON_TIME,
	BT_AUDIO_LATE,
};

struct BTAudioSource
{
	void *File;
	int32_t (*Read)( void *file, uint8_t *data, uint32_t length );
	int (*Skip)( void *file, uint32_t length );
	void *Clock;
	int (*Pace)( void *clock, uint32_t microseconds );
	struct BTAudioPlayback *Request;
	uint32_t BytesLeft;
	uint16_t Channels;
	uint8_t Live;
};

struct BTAVDTPSession
{
	struct BTL2CAPLink *Link;
	struct BTAudioSource *Source;
	struct sbc_frame Codec;
	uint16_t SignalCID;
	uint16_t MediaCID;
	uint8_t Transaction;
	uint8_t Sink;
	uint8_t MinimumBitpool;
};

enum BTHCIResult BT2_ReadWave( struct BTAudioSource *source );
int BT2_ReadAudioBytes( struct BTAudioSource *source, uint8_t *data, uint32_t length );
enum BTHCIResult BT2_PlayA2DP( const struct BTACLTransport *transport, struct BTAudioSource *source,
	const uint8_t *key, struct BTHCIError *error );
enum BTHCIResult BT2_AVDTPCommand( struct BTAVDTPSession *session, uint8_t signal, const uint8_t *data,
	uint8_t length, struct BTL2CAPPacket *response, struct BTHCIError *error );
enum BTHCIResult BT2_StreamSBC( struct BTAVDTPSession *session, struct BTHCIError *error );

#endif
