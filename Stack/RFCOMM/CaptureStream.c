
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_rfcomm.h"

// --

enum BTHCIResult
BT2_CaptureSerialFrame( struct BTRFCOMMSession *session, const struct BTRFCOMMFrame *frame,
	struct BTHCIError *error )
{
	struct BTRFCOMMSink *sink;
	struct BTRFCOMMCapture *capture;

	uint64_t left;
	uint32_t count;
	uint32_t written;
	int32_t actual;

	if ( ! session->CreditMode )
	{
		return ( BT_RFCOMM_NEEDS_CREDITS );
	}

	if ( frame->Length > session->FrameSize )
	{
		return ( BT_HCI_INVALID_REPLY );
	}

	sink = session->Sink;
	capture = sink->Capture;
	left = capture->Expected - capture->Received;
	count = frame->Length;

	if ( left < count )
	{
		count = left;
	}

	written = 0;

	while( written < count )
	{
		if ( BT2_LinkCancelled( session->Link ) )
		{
			return ( BT_RFCOMM_CANCELLED );
		}

		actual = sink->Write( sink->Context, frame->Data + written, count - written );

		if ( ( actual <= 0 ) || ( (uint32_t) actual > count - written ) )
		{
			return ( BT_RFCOMM_FILE_ERROR );
		}

		written += actual;
		capture->Received += actual;
	}

	if ( capture->Received < capture->Expected )
	{
		return ( BT2_GrantRFCOMMCredit( session, error ) );
	}

	return ( BT_HCI_OK );
}

// --
