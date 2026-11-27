
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_share.h"
#include "bt2_sdp.h"

// --

enum BTHCIResult
BT2_ShareSend ( struct BTShareSession * session, const uint8_t * data, uint16_t length, struct BTHCIError * error )
{
	enum BTHCIResult result;

	uint8_t part;
	U32		index;

	if ( ( session->RFCOMM.PeerPaused ) || ( session->RFCOMM.MuxPaused ) )
	{
		if ( ( session->PendingLength ) || ( length > sizeof ( session->Pending ) ) )
		{
			return ( BT_HCI_INVALID_REPLY );
		}

		for ( index = 0; index < length; index++ )
		{
			session->Pending[index] = data[index];
		}

		session->PendingLength = length;

		return ( BT_HCI_OK );
	}

	while ( length )
	{
		part = length > session->RFCOMM.FrameSize ? session->RFCOMM.FrameSize : length;

		result = BT2_WriteRFCOMM ( &session->RFCOMM, BT_SHARE_DLCI, BT_RFCOMM_UIH, data, part, error );

		if ( result != BT_HCI_OK )
		{
			return ( result );
		}

		data += part;
		length -= part;
	}

	return ( BT_HCI_OK );
}

// --

static enum BTHCIResult
MuxCommand ( struct BTShareSession * session, const struct BTRFCOMMFrame * frame, struct BTHCIError * error )
{
	enum BTHCIResult result;

	uint8_t	 response[127];
	uint8_t	 kind;
	U32		 index;
	U32		 length;
	uint16_t pending;

	if ( ( frame->Length < 2 ) || ( frame->Length > sizeof ( response ) ) || ( ( frame->Data[0] & 3 ) != 3 )
		 || ( ! ( frame->Data[1] & 1 ) ) || ( ( frame->Data[1] >> 1 ) != frame->Length - 2 ) )
	{
		return ( BT_HCI_OK );
	}

	for ( index = 0; index < frame->Length; index++ )
	{
		response[index] = frame->Data[index];
	}

	length = frame->Length;
	kind   = response[0] & 0xfc;
	response[0] &= ~2;

	switch ( kind )
	{
		case 0x80: // Parameter negotiation.
			if ( ( length != 10 ) || ( response[2] != BT_SHARE_DLCI ) || ( session->DLCOpen ) )
			{
				return ( BT_HCI_INVALID_REPLY );
			}

			index = BT_ReadLE16 ( &response[6] );

			if ( index < 23 )
			{
				return ( BT_HCI_INVALID_REPLY );
			}

			if ( index > session->RFCOMM.Link->PeerMTU - 4U )
			{
				index = session->RFCOMM.Link->PeerMTU - 4U;
			}

			session->RFCOMM.FrameSize = index < 127 ? index : 127;
			response[3]				  = 0; // Basic flow control.
			response[6]				  = session->RFCOMM.FrameSize;
			response[7]				  = 0;
			response[9]				  = 0;
			break;

		case 0x20: // Test.
			break;

		case 0x60: // Flow control off.
		case 0xa0: // Flow control on.
			if ( length != 2 )
			{
				return ( BT_HCI_INVALID_REPLY );
			}

			session->RFCOMM.MuxPaused = kind == 0x60;
			break;

		case 0xe0: // Modem Status.
			if ( ( length < 4 ) || ( length > 5 ) || ( response[2] != ( ( BT_SHARE_DLCI << 2 ) | 3 ) ) )
			{
				return ( BT_HCI_INVALID_REPLY );
			}

			session->RFCOMM.PeerPaused = ( response[3] & 2 ) != 0;
			break;

		default:
			response[0] = 0x11; // Non-supported command.
			response[1] = 3;
			response[2] = frame->Data[0];
			length		= 3;
			break;
	}

	result = BT2_WriteRFCOMM ( &session->RFCOMM, 0, BT_RFCOMM_UIH, response, length, error );

	if ( ( result == BT_HCI_OK ) && ( session->PendingLength ) && ( ! session->RFCOMM.PeerPaused )
		 && ( ! session->RFCOMM.MuxPaused ) )
	{
		if ( ( ( session->Access & BT2_ShareAccess ( session, NULL ) ) != session->Access )
			 || ( ! BT2_ShareEncrypted ( session->RFCOMM.Link, error ) ) )
		{
			return ( BT_L2CAP_REJECTED );
		}

		pending				   = session->PendingLength;
		session->PendingLength = 0;

		return ( BT2_ShareSend ( session, session->Pending, pending, error ) );
	}

	return ( result );
}

// --

enum BTHCIResult
BT2_ShareRFCOMM ( struct BTShareSession * session, const struct BTL2CAPPacket * packet, struct BTHCIError * error )
{
	struct BTRFCOMMFrame frame;
	enum BTHCIResult	 result;

	static const uint8_t modem[] = { 0xe3, 5, ( BT_SHARE_DLCI << 2 ) | 3, 0x8d };

	U32 index;
	U32 wanted;

	if ( ! BT2_DecodeRFCOMM ( packet, 0, &frame ) )
	{
		return ( BT_HCI_INVALID_REPLY );
	}

	if ( ( frame.Type == BT_RFCOMM_SABM ) && ( frame.Final ) && ( ! frame.Length ) )
	{
		if ( ! frame.DLCI )
		{
			session->MuxOpen = TRUE;
		}
		else if ( ( frame.DLCI == BT_SHARE_DLCI ) && ( session->MuxOpen ) )
		{
			session->DLCOpen = TRUE;

			if ( ! session->RFCOMM.FrameSize )
			{
				index					  = session->RFCOMM.Link->PeerMTU - 4U;
				session->RFCOMM.FrameSize = index < 127 ? index : 127;
			}
		}
		else
		{
			return ( BT2_WriteRFCOMM ( &session->RFCOMM, frame.DLCI, BT_RFCOMM_DM | BT_RFCOMM_F, NULL, 0, error ) );
		}

		result = BT2_WriteRFCOMM ( &session->RFCOMM, frame.DLCI, BT_RFCOMM_UA | BT_RFCOMM_F, NULL, 0, error );

		if ( ( result == BT_HCI_OK ) && ( frame.DLCI ) )
		{
			result = BT2_WriteRFCOMM ( &session->RFCOMM, 0, BT_RFCOMM_UIH, modem, sizeof ( modem ), error );
		}

		return ( result );
	}

	if ( ( frame.Type == BT_RFCOMM_DISC ) && ( ( ! frame.DLCI ) || ( frame.DLCI == BT_SHARE_DLCI ) ) )
	{
		session->DLCOpen		   = FALSE;
		session->OBEXConnected	   = FALSE;
		session->Used			   = 0;
		session->PendingLength	   = 0;
		session->RFCOMM.PeerPaused = FALSE;
		session->RFCOMM.MuxPaused  = FALSE;

		BT2_ShareClose ( session );

		if ( ! frame.DLCI )
		{
			session->MuxOpen = FALSE;
		}

		return ( BT2_WriteRFCOMM ( &session->RFCOMM, frame.DLCI, BT_RFCOMM_UA | BT_RFCOMM_F, NULL, 0, error ) );
	}

	if ( ( frame.Type != BT_RFCOMM_UIH ) || ( ! session->MuxOpen ) )
	{
		return ( BT_HCI_OK );
	}

	if ( ! frame.DLCI )
	{
		return ( MuxCommand ( session, &frame, error ) );
	}

	if ( ( frame.DLCI != BT_SHARE_DLCI ) || ( ! session->DLCOpen ) || ( frame.Length > session->RFCOMM.FrameSize ) )
	{
		return ( BT_HCI_INVALID_REPLY );
	}

	for ( index = 0; index < frame.Length; index++ )
	{
		session->Input[session->Used++] = frame.Data[index];

		if ( session->Used < 3 )
		{
			continue;
		}

		wanted = BT_ReadBE16 ( &session->Input[1] );

		if ( ( wanted < 3 ) || ( wanted > sizeof ( session->Input ) ) )
		{
			return ( BT_HCI_INVALID_REPLY );
		}

		if ( session->Used == wanted )
		{
			if ( ! BT2_ShareEncrypted ( session->RFCOMM.Link, error ) )
			{
				return ( BT_L2CAP_REJECTED );
			}

			result = BT2_ShareOBEX ( session, session->Input, wanted, error );

			session->Used = 0;

			if ( result != BT_HCI_OK )
			{
				return ( result );
			}
		}
	}

	return ( BT_HCI_OK );
}

// --
