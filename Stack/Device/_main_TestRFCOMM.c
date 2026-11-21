
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_all.h"
#include "bt2_l2cap.h"

// --

static enum BTHCIResult
_rfcomm_TestController( struct Bluetooth2RFCOMMIFace *self, U32 unit, struct BTRFCOMMTest *test, struct BTHCIError *error )
{
	struct BTControllerLocation location;
	struct BTControllerProbe probe;

	if ( ! test )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	test->Stage = BT_RFCOMM_STAGE_LINK;
	probe.Info = NULL;
	probe.Error = error;
	probe.Inquiry = NULL;
	probe.Pair = NULL;
	probe.Bond = NULL;
	probe.RemoteName = NULL;
	probe.Visibility = NULL;
	probe.EchoAddress = NULL;
	probe.Services = NULL;
	probe.RFCOMM = test;
	probe.Serial = NULL;
	probe.SerialFile = NULL;
	probe.SerialCapture = NULL;
	probe.ObjectPush = NULL;
	probe.LE = NULL;

	return ( BT2_RequestController( (PTR) self->Data.LibBase, unit, & location, & probe ) );
}

// --

static enum BTHCIResult
RunRFCOMMWithBond( struct Bluetooth2Base *base, const struct BTACLTransport *transport,
	struct BTRFCOMMTest *test, struct BTRFCOMMTransfer *transfer, struct BTHCIError *error )
{
	enum BTHCIResult result;

	uint8_t key[BT_LINK_KEY_SIZE];
	U32 byte;

	if ( ( ! test ) || ( ! test->Channel ) || ( test->Channel > 30 ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	result = BT2_LoadBondKey( base, & transport->HCI, test->Address, key, error );

	if ( ( result == BT_HCI_OK ) || ( result == BT_HCI_NO_SAVED_KEY ) )
	{
		if ( transfer )
		{
			result = BT2_ExchangeRFCOMMWithKey( transport, transfer, result == BT_HCI_OK ? key : NULL, error );
		}
		else
		{
			result = BT2_TestRFCOMMWithKey( transport, test, result == BT_HCI_OK ? key : NULL, error );
		}
	}

	for( byte = 0; byte < sizeof( key ); byte++ )
	{
		((volatile uint8_t *)key)[byte] = 0;
	}

	return ( result );
}

// --

static enum BTHCIResult
_rfcomm_Test( struct Bluetooth2RFCOMMIFace *self, const struct BTACLTransport *transport,
	struct BTRFCOMMTest *test, struct BTHCIError *error )
{
	return ( RunRFCOMMWithBond( (PTR) self->Data.LibBase, transport, test, NULL, error ) );
}

// --
