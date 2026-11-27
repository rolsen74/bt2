
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_device.h"
#include "bt2_hid.h"

// --

enum BTHCIResult
_main_hidinput_Connect ( struct Bluetooth2IFace * self, const struct BTACLTransport * transport, struct BTHIDInput * input,
						 struct BTHCIError * error )
{
	struct Bluetooth2Base * base;
	enum BTHCIResult		result;

	const char * phase;
	uint8_t		 key[BT_LINK_KEY_SIZE];
	U32			 byte;

	if ( ! ValidHIDInput ( input ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	base  = (PTR)self->Data.LibBase;
	phase = NULL;

	result = BT2_LoadBondKey ( base, &transport->HCI, input->Monitor.Address, key, error );

	if ( result == BT_HCI_OK )
	{
		result = BT2_ConnectHIDInput ( transport, input, key, &phase, error );
	}

	for ( byte = 0; byte < sizeof ( key ); byte++ )
	{
		( (volatile uint8_t *)key )[byte] = 0;
	}

	if ( ( phase ) && ( result == BT_ACL_READ_ERROR ) && ( error->TransportError == BT_HCI_TRANSPORT_TIMEOUT ) )
	{
		base->Exec->DebugPrintF ( "Bluetooth HID: timed out during %s\n", phase );
	}

	return ( result );
}

// --
