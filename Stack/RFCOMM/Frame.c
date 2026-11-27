
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <stddef.h>
#include "bt2_rfcomm.h"

// --

static uint8_t
FrameFCS ( const uint8_t * data, uint8_t size )
{
	uint8_t fcs;
	uint8_t bit;

	fcs = 0xff;

	while ( size-- )
	{
		fcs ^= *data++;

		for ( bit = 0; bit < 8; bit++ )
		{
			fcs = ( fcs >> 1 ) ^ ( ( fcs & 1 ) ? 0xe0 : 0 );
		}
	}

	return ( 0xff - fcs );
}

// --

static enum BTHCIResult
WriteFrame ( struct BTRFCOMMSession * session, uint8_t dlci, uint8_t control, const uint8_t * data, uint8_t size, uint8_t credit,
			 struct BTHCIError * error )
{
	uint8_t	 frame[5 + 127];
	uint32_t byte;
	uint32_t header;
	uint8_t	 type;

	if ( size > 127 )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	type	 = control & ~BT_RFCOMM_F;
	frame[0] = ( dlci << 2 ) | ( ( ( type == BT_RFCOMM_UA ) || ( type == BT_RFCOMM_DM ) ) ? 1 : 3 );
	frame[0] ^= session->Responder ? 2 : 0;
	frame[1] = control;
	frame[2] = ( size << 1 ) | 1;
	header	 = 3;

	if ( credit )
	{
		frame[header++] = credit;
	}

	for ( byte = 0; byte < size; byte++ )
	{
		frame[header + byte] = data[byte];
	}

	frame[header + size] = FrameFCS ( frame, ( ( control & ~BT_RFCOMM_F ) == BT_RFCOMM_UIH ) ? 2 : 3 );

	return ( BT2_WriteL2CAP ( session->Link, session->RemoteCID, frame, header + 1 + size, error ) );
}

// --

enum BTHCIResult
BT2_WriteRFCOMM ( struct BTRFCOMMSession * session, uint8_t dlci, uint8_t control, const uint8_t * data, uint8_t size,
				  struct BTHCIError * error )
{
	return ( WriteFrame ( session, dlci, control, data, size, 0, error ) );
}

// --

enum BTHCIResult
BT2_GrantRFCOMMCredit ( struct BTRFCOMMSession * session, struct BTHCIError * error )
{
	return ( WriteFrame ( session, session->DLCI, BT_RFCOMM_UIH | BT_RFCOMM_F, NULL, 0, 1, error ) );
}

// --

int
BT2_DecodeRFCOMM ( const struct BTL2CAPPacket * pdu, uint8_t creditMode, struct BTRFCOMMFrame * frame )
{
	const uint8_t * data;
	uint32_t		length;
	uint8_t			header;
	uint8_t			credit;

	data = pdu->Data + BT_L2CAP_HEADER;

	if ( ( pdu->Length < 4 ) || ( ! ( data[0] & 1 ) ) )
	{
		return ( 0 );
	}

	header = 3;
	length = data[2] >> 1;

	if ( ! ( data[2] & 1 ) )
	{
		header = 4;
		length |= (uint32_t)data[3] << 7;
	}

	frame->DLCI	 = data[0] >> 2;
	frame->Type	 = data[1] & ~BT_RFCOMM_F;
	frame->Final = ( data[1] & BT_RFCOMM_F ) != 0;
	credit		 = ( ( creditMode ) && ( frame->Type == BT_RFCOMM_UIH ) && ( frame->DLCI ) && ( frame->Final ) ) ? 1 : 0;

	if ( header + credit + length + 1 != pdu->Length )
	{
		return ( 0 );
	}

	if ( FrameFCS ( data, frame->Type == BT_RFCOMM_UIH ? 2 : header ) != data[header + credit + length] )
	{
		return ( 0 );
	}

	frame->Credits = credit ? data[header] : 0;
	frame->Data	   = data + header + credit;
	frame->Length  = length;

	return ( 1 );
}

// --
