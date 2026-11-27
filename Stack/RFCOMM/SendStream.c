
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_rfcomm.h"

// --

enum BTHCIResult
BT2_SendSerialStream ( struct BTRFCOMMSession * session, struct BTHCIError * error )
{
	struct BTRFCOMMStream * stream;
	enum BTHCIResult		result;

	uint8_t	 data[4096];
	int32_t	 length;
	uint32_t sent;

	stream = session->Stream;

	if ( ! session->CreditMode )
	{
		return ( BT_RFCOMM_NEEDS_CREDITS );
	}

	for ( ;; )
	{
		if ( BT2_LinkCancelled ( session->Link ) )
		{
			return ( BT_RFCOMM_CANCELLED );
		}

		length = stream->Read ( stream->Context, data, sizeof ( data ) );

		if ( ( length < 0 ) || ( (uint32_t)length > sizeof ( data ) ) )
		{
			return ( BT_RFCOMM_FILE_ERROR );
		}

		if ( ! length )
		{
			return ( BT_HCI_OK );
		}

		sent = 0;

		result = BT2_WriteSerialData ( session, data, length, &sent, error );

		stream->File->Sent += sent;

		if ( result != BT_HCI_OK )
		{
			return ( result );
		}
	}
}

// --
