
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_device.h"
#include "bt2_l2cap.h"

// --

enum BTHCIResult
RunRFCOMMWithBond ( struct Bluetooth2Base * base, const struct BTACLTransport * transport, struct BTRFCOMMTest * test,
					struct BTRFCOMMTransfer * transfer, struct BTHCIError * error )
{
	enum BTHCIResult result;

	uint8_t key[BT_LINK_KEY_SIZE];
	U32		byte;

	if ( ( ! test ) || ( ! test->Channel ) || ( test->Channel > 30 ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	result = BT2_LoadBondKey ( base, &transport->HCI, test->Address, key, error );

	if ( ( result == BT_HCI_OK ) || ( result == BT_HCI_NO_SAVED_KEY ) )
	{
		if ( transfer )
		{
			result = BT2_ExchangeRFCOMMWithKey ( transport, transfer, result == BT_HCI_OK ? key : NULL, error );
		}
		else
		{
			result = BT2_TestRFCOMMWithKey ( transport, test, result == BT_HCI_OK ? key : NULL, error );
		}
	}

	for ( byte = 0; byte < sizeof ( key ); byte++ )
	{
		( (volatile uint8_t *)key )[byte] = 0;
	}

	return ( result );
}

// --
