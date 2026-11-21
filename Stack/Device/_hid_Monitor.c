
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
_hid_Monitor( struct Bluetooth2HIDIFace *self, const struct BTACLTransport *transport,
	struct BTHIDMonitor *monitor, struct BTHCIError *error )
{
	struct Bluetooth2Base *base;
	struct BTDeadline timer;
	enum BTHCIResult result;

	uint8_t key[BT_LINK_KEY_SIZE];
	U32 byte;

	if ( ! ValidHIDMonitor( monitor ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	base = (PTR) self->Data.LibBase;

	result = OpenDeadline( base, & timer, monitor->Seconds );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	result = BT2_LoadBondKey( base, & transport->HCI, monitor->Address, key, error );

	if ( ( result == BT_HCI_OK ) || ( result == BT_HCI_NO_SAVED_KEY ) )
	{
		result = BT2_MonitorHID( transport, monitor, result == BT_HCI_OK ? key : NULL, DeadlineExpired, & timer, error );
	}

	for( byte = 0; byte < sizeof( key ); byte++ )
	{
		((volatile uint8_t *) key)[byte] = 0;
	}

	CloseDeadline( base, & timer );

	return ( result );
}

// --
