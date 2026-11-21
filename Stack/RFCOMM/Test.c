
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <stddef.h>
#include "bt2_obex.h"

// --

#define RFCOMM_PSM 0x0003
#define RFCOMM_LOCAL_CID 0x0040
#define RFCOMM_PN_COMMAND 0x83
#define RFCOMM_MSC_COMMAND 0xe3

// --

static enum BTHCIResult
ControlFrame( struct BTRFCOMMSession *session, uint8_t dlci, uint8_t control, struct BTHCIError *error )
{
	enum BTHCIResult result;

	result = BT2_WriteRFCOMM( session, dlci, control | BT_RFCOMM_F, NULL, 0, error );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	return ( BT2_WaitRFCOMM( session, BT_RFCOMM_WAIT_ACK, dlci, error ) );
}

// --

static enum BTHCIResult
RunSession( struct BTRFCOMMSession *session, struct BTRFCOMMTest *test, struct BTHCIError *error )
{
	enum BTHCIResult result;

	uint8_t pn[10];
	uint8_t msc[4];

	test->Stage = BT_RFCOMM_STAGE_MULTIPLEXER;

	result = ControlFrame( session, 0, BT_RFCOMM_SABM, error );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	test->Stage = BT_RFCOMM_STAGE_PARAMETERS;
	pn[0] = RFCOMM_PN_COMMAND;
	pn[1] = 0x11;
	pn[2] = session->DLCI;
	pn[3] = 0xf0;
	pn[4] = 7;
	pn[5] = 0;
	pn[6] = session->FrameSize;
	pn[7] = 0;
	pn[8] = 0;
	pn[9] = 7;

	result = BT2_WriteRFCOMM( session, 0, BT_RFCOMM_UIH, pn, sizeof( pn ), error );

	if ( result == BT_HCI_OK )
	{
		result = BT2_WaitRFCOMM( session, BT_RFCOMM_WAIT_PN, 0, error );
	}

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	test->Stage = BT_RFCOMM_STAGE_CHANNEL;

	result = ControlFrame( session, session->DLCI, BT_RFCOMM_SABM, error );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	test->Stage = BT_RFCOMM_STAGE_MODEM;
	msc[0] = RFCOMM_MSC_COMMAND;
	msc[1] = 0x05;
	msc[2] = ( session->DLCI << 2 ) | 3;
	msc[3] = 0x8d;

	result = BT2_WriteRFCOMM( session, 0, BT_RFCOMM_UIH, msc, sizeof( msc ), error );

	if ( result == BT_HCI_OK )
	{
		result = BT2_WaitRFCOMM( session, BT_RFCOMM_WAIT_MSC, 0, error );
	}

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	if ( session->Push )
	{
		test->Stage = BT_RFCOMM_STAGE_DATA;

		result = BT2_RunObjectPush( session, error );

		if ( result != BT_HCI_OK )
		{
			return ( result );
		}
	}
	else if ( session->Stream )
	{
		test->Stage = BT_RFCOMM_STAGE_DATA;

		result = BT2_SendSerialStream( session, error );

		if ( result != BT_HCI_OK )
		{
			return ( result );
		}
	}
	else if ( session->Sink )
	{
		test->Stage = BT_RFCOMM_STAGE_DATA;

		if ( ! session->CreditMode )
		{
			return ( BT_RFCOMM_NEEDS_CREDITS );
		}

		result = BT2_WaitRFCOMM( session, BT_RFCOMM_WAIT_CAPTURE, 0, error );

		if ( result != BT_HCI_OK )
		{
			return ( result );
		}
	}
	else if ( session->Transfer )
	{
		test->Stage = BT_RFCOMM_STAGE_DATA;

		result = BT2_ExchangeSerialData( session, error );

		if ( result != BT_HCI_OK )
		{
			return ( result );
		}
	}

	test->Stage = BT_RFCOMM_STAGE_CLOSE;

	result = ControlFrame( session, session->DLCI, BT_RFCOMM_DISC, error );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	return ( ControlFrame( session, 0, BT_RFCOMM_DISC, error ) );
}

// --

static enum BTHCIResult
TestSession( const struct BTACLTransport *transport, struct BTRFCOMMTest *test,
	struct BTRFCOMMTransfer *transfer, struct BTRFCOMMStream *stream, struct BTRFCOMMSink *sink,
	struct BTOBEXPush *push, const uint8_t *key, struct BTHCIError *error )
{
	struct BTL2CAPLink link;
	struct BTRFCOMMSession session;
	enum BTHCIResult result;

	const uint32_t *cancel;

	if ( ( ! test ) || ( ! test->Channel ) || ( test->Channel > 30 ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	test->Stage = BT_RFCOMM_STAGE_LINK;

	cancel = stream ? & stream->File->Cancel : NULL;

	if ( sink )
	{
		cancel = & sink->Capture->Cancel;
	}
	else if ( push )
	{
		cancel = & push->Request->Cancel;
	}

	if ( ( cancel ) && ( __atomic_load_n( cancel, __ATOMIC_RELAXED ) ) )
	{
		return ( BT_RFCOMM_CANCELLED );
	}

	result = BT2_OpenDataLinkWithKey( & link, transport, test->Address, key, error );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	test->Stage = BT_RFCOMM_STAGE_L2CAP;
	session.Link = & link;
	session.Transfer = transfer;
	session.Stream = stream;
	session.Sink = sink;
	session.Push = push;
	link.Cancel = cancel;
	session.SendCredits = 0;
	session.CreditMode = 0;
	session.PeerModemSeen = 0;
	session.PeerPaused = 0;
	session.MuxPaused = 0;
	session.LocalCID = RFCOMM_LOCAL_CID;
	session.DLCI = test->Channel << 1;

	result = BT2_OpenChannel( & link, RFCOMM_PSM, session.LocalCID, & session.RemoteCID, error );

	if ( result == BT_HCI_OK )
	{
		session.FrameSize = link.PeerMTU < 131 ? link.PeerMTU - 4 : 127;

		result = RunSession( & session, test, error );
	}

	return ( BT2_CloseDataLink( & link, result, error ) );
}

// --

enum BTHCIResult
BT2_TestRFCOMMWithKey( const struct BTACLTransport *transport, struct BTRFCOMMTest *test,
	const uint8_t *key, struct BTHCIError *error )
{
	return ( TestSession( transport, test, NULL, NULL, NULL, NULL, key, error ) );
}

// --

enum BTHCIResult
BT2_ExchangeRFCOMMWithKey( const struct BTACLTransport *transport, struct BTRFCOMMTransfer *transfer,
	const uint8_t *key, struct BTHCIError *error )
{
	if ( ( ! transfer ) || ( transfer->Length > BT_RFCOMM_TEST_DATA_MAX ) ||
		( ( transfer->Length ) && ( ! transfer->Data ) ) ||
		( ( transfer->ReplyCapacity ) && ( ! transfer->Reply ) ) ||
		( ( ! transfer->Length ) && ( ! transfer->ReplyCapacity ) ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	transfer->Sent = 0;
	transfer->ReplyLength = 0;
	transfer->ReplyTruncated = 0;

	return ( TestSession( transport, & transfer->Peer, transfer, NULL, NULL, NULL, key, error ) );
}

// --

enum BTHCIResult
BT2_SendRFCOMMStream( const struct BTACLTransport *transport, struct BTRFCOMMStream *stream,
	const uint8_t *key, struct BTHCIError *error )
{
	if ( ( ! stream ) || ( ! stream->Read ) || ( ! stream->File ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	stream->File->Sent = 0;

	return ( TestSession( transport, & stream->File->Peer, NULL, stream, NULL, NULL, key, error ) );
}

// --

enum BTHCIResult
BT2_CaptureRFCOMMStream( const struct BTACLTransport *transport, struct BTRFCOMMSink *sink,
	const uint8_t *key, struct BTHCIError *error )
{
	enum BTHCIResult result;

	if ( ( ! sink ) || ( ! sink->Write ) || ( ! sink->Capture ) || ( ! sink->Capture->Expected ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	sink->Capture->Received = 0;

	result = TestSession( transport, & sink->Capture->Peer, NULL, NULL, sink, NULL, key, error );

	// The peer may close the channel as soon as it has sent the requested bytes.
	if ( ( sink->Capture->Peer.Stage == BT_RFCOMM_STAGE_CLOSE ) &&
		( sink->Capture->Received == sink->Capture->Expected ) && ( result != BT_RFCOMM_CANCELLED ) )
	{
		return ( BT_HCI_OK );
	}

	return ( result );
}

// --

enum BTHCIResult
BT2_PushObject( const struct BTACLTransport *transport, struct BTOBEXPush *push, const uint8_t *key,
	struct BTHCIError *error )
{
	enum BTHCIResult result;

	if ( ( ! push ) || ( ! push->Request ) || ( ! push->Name ) || ( ! push->Read ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	push->AwaitingReply = 0;
	push->Accepted = 0;
	push->Request->Sent = 0;
	push->Request->Response = 0;

	result = TestSession( transport, & push->Request->Peer, NULL, NULL, NULL, push, key, error );

	return ( push->Accepted ? BT_HCI_OK : result );
}

// --

enum BTHCIResult
BT_ExchangeRFCOMM( const struct BTACLTransport *transport, struct BTRFCOMMTransfer *transfer, struct BTHCIError *error )
{
	return ( BT2_ExchangeRFCOMMWithKey( transport, transfer, NULL, error ) );
}

// --

enum BTHCIResult
BT_TestRFCOMM( const struct BTACLTransport *transport, struct BTRFCOMMTest *test, struct BTHCIError *error )
{
	return ( BT2_TestRFCOMMWithKey( transport, test, NULL, error ) );
}

// --
