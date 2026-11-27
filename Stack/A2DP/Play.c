
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <stddef.h>
#include "bt2_a2dp.h"

// --

static int
ReadCapabilities ( struct BTAVDTPSession * session, const struct BTL2CAPPacket * response, uint8_t * configuration )
{
	const uint8_t * data;
	uint32_t		remaining;
	uint32_t		length;
	uint8_t			frequency;
	uint8_t			modes;
	uint8_t			mode;
	uint8_t			transport;
	uint8_t			codec;
	uint8_t			limit;

	frequency = session->Source->Request->Rate == 44100 ? 0x20 : 0x10;
	transport = 0;
	codec	  = 0;
	data	  = response->Data + BT_L2CAP_HEADER + 2;
	remaining = response->Length - 2;

	while ( remaining >= 2 )
	{
		length = data[1];

		if ( length > remaining - 2 )
		{
			return ( 0 );
		}

		if ( ( data[0] == 1 ) && ( ! length ) )
		{
			transport = 1;
		}
		else if ( ( data[0] == 7 ) && ( length == 6 ) && ( ! data[2] ) && ( ! data[3] ) && ( data[4] & frequency )
				  && ( ( data[5] & 0x15 ) == 0x15 ) )
		{
			modes = data[4];

			if ( ( session->Source->Channels == 1 ) && ( modes & 8 ) )
			{
				mode				= 8;
				session->Codec.mode = SBC_MODE_MONO;
			}
			else if ( modes & 1 )
			{
				mode				= 1;
				session->Codec.mode = SBC_MODE_JOINT_STEREO;
			}
			else if ( modes & 2 )
			{
				mode				= 2;
				session->Codec.mode = SBC_MODE_STEREO;
			}
			else if ( modes & 8 )
			{
				mode				= 8;
				session->Codec.mode = SBC_MODE_MONO;
			}
			else
			{
				return ( 0 );
			}

			limit					= mode == 8 ? 26 : 35;
			session->MinimumBitpool = data[6] > 2 ? data[6] : 2;
			session->Codec.bitpool	= data[7] < limit ? data[7] : limit;

			if ( session->Codec.bitpool < session->MinimumBitpool )
			{
				return ( 0 );
			}

			configuration[0] = frequency | mode;
			configuration[1] = 0x15;
			configuration[2] = session->MinimumBitpool;
			configuration[3] = session->Codec.bitpool;
			codec			 = 1;
		}

		remaining -= length + 2;
		data += length + 2;
	}

	return ( ( ! remaining ) && ( transport ) && ( codec ) );
}

// --

static enum BTHCIResult
SelectSink ( struct BTAVDTPSession * session, uint8_t * configuration, struct BTHCIError * error )
{
	struct BTL2CAPPacket discover;
	struct BTL2CAPPacket capabilities;
	enum BTHCIResult	 result;

	const uint8_t * data;
	uint32_t		offset;
	uint8_t			endpoint;

	result = BT2_AVDTPCommand ( session, 1, NULL, 0, &discover, error );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	if ( discover.Length & 1 )
	{
		return ( BT_HCI_INVALID_REPLY );
	}

	data = discover.Data + BT_L2CAP_HEADER;

	for ( offset = 2; offset < discover.Length; offset += 2 )
	{
		endpoint = data[offset] & 0xfc;

		if ( ( ! endpoint ) || ( endpoint > 0xf8 ) || ( data[offset] & 2 ) || ( ( data[offset + 1] & 0xf8 ) != 8 ) )
		{
			continue;
		}

		result = BT2_AVDTPCommand ( session, 2, &endpoint, 1, &capabilities, error );

		if ( result == BT_A2DP_REJECTED )
		{
			continue;
		}

		if ( result != BT_HCI_OK )
		{
			return ( result );
		}

		if ( ReadCapabilities ( session, &capabilities, configuration ) )
		{
			session->Sink = endpoint;

			return ( BT_HCI_OK );
		}
	}

	return ( BT_A2DP_NO_SINK );
}

// --

static enum BTHCIResult
PlayOnLink ( struct BTAVDTPSession * session, struct BTHCIError * error )
{
	struct BTL2CAPPacket response;
	struct BTHCIError	 cleanupError;
	enum BTHCIResult	 result;

	uint8_t configuration[12] = { 0, 4, 1, 0, 7, 6, 0, 0 };

	result = BT2_OpenChannel ( session->Link, BT_AVDTP_PSM, BT_AVDTP_SIGNAL_CID, &session->SignalCID, error );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	result = SelectSink ( session, &configuration[8], error );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	configuration[0] = session->Sink;

	result = BT2_AVDTPCommand ( session, 3, configuration, sizeof ( configuration ), &response, error );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	result = BT2_AVDTPCommand ( session, 6, &session->Sink, 1, &response, error );

	if ( result == BT_HCI_OK )
	{
		session->Link->PeerMTU = BT_L2CAP_MTU;

		result = BT2_OpenChannel ( session->Link, BT_AVDTP_PSM, BT_AVDTP_MEDIA_CID, &session->MediaCID, error );
	}

	if ( result == BT_HCI_OK )
	{
		result = BT2_AVDTPCommand ( session, 7, &session->Sink, 1, &response, error );
	}

	if ( result == BT_HCI_OK )
	{
		result = BT2_StreamSBC ( session, error );
	}

	if ( result == BT_HCI_OK )
	{
		result = BT2_AVDTPCommand ( session, 8, &session->Sink, 1, &response, error );
	}
	else if ( ( result != BT_RFCOMM_CANCELLED ) && ( ! BT2_LinkCancelled ( session->Link ) ) )
	{
		uint8_t failedSignal;
		uint8_t failedStatus;

		failedSignal = session->Source->Request->Signal;
		failedStatus = session->Source->Request->Status;

		BT2_AVDTPCommand ( session, 10, &session->Sink, 1, &response, &cleanupError );

		session->Source->Request->Signal = failedSignal;
		session->Source->Request->Status = failedStatus;
	}

	return ( result );
}

// --

enum BTHCIResult
BT2_PlayA2DP ( const struct BTACLTransport * transport, struct BTAudioSource * source, const uint8_t * key,
			   struct BTHCIError * error )
{
	struct BTAVDTPSession session;
	struct BTL2CAPLink	  link;
	enum BTHCIResult	  result;

	if ( __atomic_load_n ( &source->Request->Cancel, __ATOMIC_RELAXED ) )
	{
		return ( BT_RFCOMM_CANCELLED );
	}

	result = BT2_OpenDataLinkWithPackets ( &link, transport, source->Request->Address, key, BT_ACL_PACKETS_AUDIO, error );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	link.Cancel				= &source->Request->Cancel;
	session.Link			= &link;
	session.Source			= source;
	session.Transaction		= 0;
	session.Sink			= 0;
	session.Codec.msbc		= 0;
	session.Codec.freq		= source->Request->Rate == 44100 ? SBC_FREQ_44K1 : SBC_FREQ_48K;
	session.Codec.bam		= SBC_BAM_LOUDNESS;
	session.Codec.nblocks	= 16;
	session.Codec.nsubbands = 8;

	result = PlayOnLink ( &session, error );

	return ( BT2_CloseDataLink ( &link, result, error ) );
}

// --
