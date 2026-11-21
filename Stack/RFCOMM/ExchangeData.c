
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
BT2_WriteSerialData( struct BTRFCOMMSession *session, const uint8_t *data, uint32_t length,
	uint32_t *sent, struct BTHCIError *error )
{
	enum BTHCIResult result;

	uint32_t frameLength;

	// Basic mode has no credits. Limit the test to one negotiated frame.
	if ( ( ! session->CreditMode ) && ( length > session->FrameSize ) )
	{
		error->Status = session->FrameSize;

		return ( BT_RFCOMM_TX_LIMIT );
	}

	while( *sent < length )
	{
		result = BT2_WaitRFCOMM( session, BT_RFCOMM_WAIT_WRITABLE, 0, error );

		if ( result != BT_HCI_OK )
		{
			return ( result );
		}

		frameLength = length - *sent;

		if ( frameLength > session->FrameSize )
		{
			frameLength = session->FrameSize;
		}

		result = BT2_WriteRFCOMM( session, session->DLCI, BT_RFCOMM_UIH, data + *sent, frameLength, error );

		if ( result != BT_HCI_OK )
		{
			return ( result );
		}

		*sent += frameLength;

		if ( session->CreditMode )
		{
			session->SendCredits--;
		}
	}

	return ( BT_HCI_OK );
}

// --

enum BTHCIResult
BT2_ExchangeSerialData( struct BTRFCOMMSession *session, struct BTHCIError *error )
{
	struct BTRFCOMMTransfer *transfer;
	enum BTHCIResult result;

	transfer = session->Transfer;

	result = BT2_WriteSerialData( session, transfer->Data, transfer->Length, & transfer->Sent, error );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	if ( transfer->ReplyCapacity )
	{
		return ( BT2_WaitRFCOMM( session, BT_RFCOMM_WAIT_REPLY, 0, error ) );
	}

	return ( BT_HCI_OK );
}

// --
