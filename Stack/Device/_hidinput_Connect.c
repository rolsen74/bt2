
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_all.h"
#include "bt2_hid.h"

// --

static enum BTHCIResult
_hidinput_Connect( struct Bluetooth2HIDInputIFace *self, const struct BTACLTransport *transport,
	struct BTHIDInput *input, struct BTHCIError *error )
{
	struct Bluetooth2Base *base;
	enum BTHCIResult result;

	uint8_t key[BT_LINK_KEY_SIZE];
	U32 byte;

	if ( ! ValidHIDInput( input ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	base = (PTR) self->Data.LibBase;

	result = BT2_LoadBondKey( base, & transport->HCI, input->Monitor.Address, key, error );

	if ( result == BT_HCI_OK )
	{
		result = BT2_ConnectHIDInput( transport, input, key, error );
	}

	for( byte = 0; byte < sizeof( key ); byte++ )
	{
		((volatile uint8_t *) key)[byte] = 0;
	}

	return ( result );
}

// --
