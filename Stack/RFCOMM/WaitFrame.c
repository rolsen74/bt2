
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <stddef.h>
#include "bt2_obex.h"

// --

#define RFCOMM_PN_RESPONSE	   0x81
#define RFCOMM_MSC_COMMAND	   0xe3
#define RFCOMM_MSC_RESPONSE	   0xe1
#define RFCOMM_MCC_UNSUPPORTED 0x11
#define RFCOMM_FCON			   0xa3
#define RFCOMM_FCOFF		   0x63
#define RFCOMM_V24_FC		   0x02

// --

static void
ReceiveSerialData ( struct BTRFCOMMSession * session, const struct BTRFCOMMFrame * frame )
{
	struct BTRFCOMMTransfer * transfer;

	uint32_t byte;
	uint32_t credits;

	credits				 = session->SendCredits + (uint32_t)frame->Credits;
	session->SendCredits = credits > 65535 ? 65535 : credits;
	transfer			 = session->Transfer;

	if ( ( ! transfer ) || ( ! transfer->ReplyCapacity ) || ( transfer->ReplyLength ) || ( ! frame->Length ) )
	{
		return;
	}

	transfer->ReplyLength = frame->Length;

	if ( transfer->ReplyLength > transfer->ReplyCapacity )
	{
		transfer->ReplyLength	 = transfer->ReplyCapacity;
		transfer->ReplyTruncated = 1;
	}

	for ( byte = 0; byte < transfer->ReplyLength; byte++ )
	{
		transfer->Reply[byte] = frame->Data[byte];
	}
}

// --

enum BTHCIResult
BT2_WaitRFCOMM ( struct BTRFCOMMSession * session, enum BTRFCOMMWait condition, uint8_t dlci, struct BTHCIError * error )
{
	struct BTL2CAPPacket pdu;
	struct BTL2CAPSignal signal;
	struct BTRFCOMMFrame frame;
	enum BTHCIResult	 result;

	const uint8_t * data;
	uint8_t			reply[5];
	uint32_t		length;
	uint32_t		reads;
	uint8_t			mccLength;
	uint16_t		frameSize;

	reads = 256;

	for ( ;; )
	{
		if ( BT2_LinkCancelled ( session->Link ) )
		{
			return ( BT_RFCOMM_CANCELLED );
		}

		if ( ( ( condition == BT_RFCOMM_WAIT_WRITABLE ) && ( session->PeerModemSeen )
			   && ( session->CreditMode ? session->SendCredits != 0 : ! ( session->PeerPaused || session->MuxPaused ) ) )
			 || ( ( condition == BT_RFCOMM_WAIT_REPLY ) && ( session->Transfer->ReplyLength ) )
			 || ( ( condition == BT_RFCOMM_WAIT_CAPTURE )
				  && ( session->Sink->Capture->Received == session->Sink->Capture->Expected ) )
			 || ( ( condition == BT_RFCOMM_WAIT_OBEX ) && ( BT2_HasOBEXReply ( session->Push ) ) ) )
		{
			return ( BT_HCI_OK );
		}

		if ( ! reads )
		{
			return ( BT_HCI_INVALID_REPLY );
		}

		result = BT2_ReadL2CAP ( session->Link, &pdu, &reads, error );

		if ( result != BT_HCI_OK )
		{
			return ( result );
		}

		data = pdu.Data + BT_L2CAP_HEADER;

		if ( pdu.CID == BT_L2CAP_SIGNAL_CID )
		{
			length = pdu.Length;

			while ( BT2_DecodeSignal ( &data, &length, &signal ) )
			{
				result = BT2_ServiceSignal ( session->Link, &signal, error );

				if ( result != BT_HCI_OK )
				{
					return ( result );
				}
			}

			continue;
		}

		if ( ( pdu.CID != session->LocalCID ) || ( ! BT2_DecodeRFCOMM ( &pdu, session->CreditMode, &frame ) ) )
		{
			continue;
		}

		if ( ( frame.DLCI ) && ( frame.DLCI != session->DLCI ) )
		{
			continue;
		}

		if ( frame.Type == BT_RFCOMM_DM )
		{
			error->Status = frame.Type;

			return ( BT_RFCOMM_REJECTED );
		}

		if ( ( frame.Type == BT_RFCOMM_DISC ) && ( ! frame.Length ) )
		{
			result = BT2_WriteRFCOMM ( session, frame.DLCI, BT_RFCOMM_UA | ( frame.Final ? BT_RFCOMM_F : 0 ), NULL, 0, error );

			if ( result != BT_HCI_OK )
			{
				return ( result );
			}

			error->Status = frame.Type;

			return ( BT_RFCOMM_REJECTED );
		}

		if ( ( condition == BT_RFCOMM_WAIT_ACK ) && ( frame.DLCI == dlci ) && ( frame.Type == BT_RFCOMM_UA ) && ( frame.Final )
			 && ( ! frame.Length ) )
		{
			return ( BT_HCI_OK );
		}

		if ( ( frame.Type == BT_RFCOMM_UIH ) && ( frame.DLCI ) )
		{
			ReceiveSerialData ( session, &frame );

			if ( ( session->Push ) && ( session->Push->AwaitingReply ) && ( frame.Length ) )
			{
				result = BT2_ReceiveOBEX ( session, frame.Data, frame.Length, error );

				if ( result != BT_HCI_OK )
				{
					return ( result );
				}
			}

			if ( ( session->Sink ) && ( frame.Length ) )
			{
				result = BT2_CaptureSerialFrame ( session, &frame, error );

				if ( result != BT_HCI_OK )
				{
					return ( result );
				}

				if ( condition == BT_RFCOMM_WAIT_CAPTURE )
				{
					reads = 256;
				}
			}

			continue;
		}

		if ( ( frame.Type != BT_RFCOMM_UIH ) || ( frame.DLCI ) || ( frame.Length < 2 ) || ( ! ( frame.Data[1] & 1 ) ) )
		{
			continue;
		}

		mccLength = frame.Data[1] >> 1;

		if ( frame.Length != 2U + mccLength )
		{
			continue;
		}

		data = frame.Data;

		if ( ( data[0] == RFCOMM_MSC_COMMAND ) && ( ( mccLength == 2 ) || ( mccLength == 3 ) )
			 && ( ( data[2] >> 2 ) == session->DLCI ) )
		{
			session->PeerModemSeen = 1;
			session->PeerPaused	   = ( data[3] & RFCOMM_V24_FC ) != 0;
			reply[0]			   = RFCOMM_MSC_RESPONSE;
			reply[1]			   = data[1];
			reply[2]			   = data[2];
			reply[3]			   = data[3];

			if ( mccLength == 3 )
			{
				reply[4] = data[4];
			}

			result = BT2_WriteRFCOMM ( session, 0, BT_RFCOMM_UIH, reply, 2 + mccLength, error );

			if ( result != BT_HCI_OK )
			{
				return ( result );
			}
		}

		if ( ( ( data[0] == RFCOMM_FCOFF ) || ( data[0] == RFCOMM_FCON ) ) && ( ! mccLength ) )
		{
			session->MuxPaused = data[0] == RFCOMM_FCOFF;
			reply[0]		   = data[0] & ~2;
			reply[1]		   = 1;

			result = BT2_WriteRFCOMM ( session, 0, BT_RFCOMM_UIH, reply, 2, error );

			if ( result != BT_HCI_OK )
			{
				return ( result );
			}
		}

		if ( ( ( condition == BT_RFCOMM_WAIT_PN ) || ( condition == BT_RFCOMM_WAIT_MSC ) )
			 && ( data[0] == RFCOMM_MCC_UNSUPPORTED ) )
		{
			error->Status = data[0];

			return ( BT_RFCOMM_REJECTED );
		}

		if ( ( condition == BT_RFCOMM_WAIT_PN ) && ( data[0] == RFCOMM_PN_RESPONSE ) && ( mccLength == 8 )
			 && ( data[2] == session->DLCI ) )
		{
			frameSize = BT_ReadLE16 ( data + 6 );

			if ( ( ( data[3] != 0 ) && ( data[3] != 0xe0 ) ) || ( ! frameSize ) )
			{
				return ( BT_HCI_INVALID_REPLY );
			}

			if ( frameSize < session->FrameSize )
			{
				session->FrameSize = frameSize;
			}

			session->CreditMode	 = data[3] == 0xe0;
			session->SendCredits = session->CreditMode ? data[9] & 7 : 0;

			return ( BT_HCI_OK );
		}

		if ( ( condition == BT_RFCOMM_WAIT_MSC ) && ( data[0] == RFCOMM_MSC_RESPONSE ) && ( mccLength >= 2 )
			 && ( ( data[2] >> 2 ) == session->DLCI ) )
		{
			return ( BT_HCI_OK );
		}
	}
}

// --
