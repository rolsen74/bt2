
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_all.h"

// --

static S32
RegisterController( struct Bluetooth2Base *base, struct MsgPort *port,
	const struct BTControllerLocation *location, S32 capabilities )
{
	struct BT2MasterRequest request;

	request.Port = port;
	request.Location = location;
	request.Operation = BT2_MASTER_ADD;
	request.Capabilities = capabilities;

	if ( BT2_MasterCall( base, & request ) )
	{
		return ( request.Result );
	}

	return ( BT2_AddController( base, port, location, capabilities ) );
}

// --

static S32
_main_AddController( struct Bluetooth2IFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location )
{
	return ( RegisterController( (PTR) self->Data.LibBase, port, location, 0 ) );
}

// --

static S32
_main_AddDiscoveryController( struct Bluetooth2IFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location )
{
	return ( RegisterController( (PTR) self->Data.LibBase, port, location, BT2_CAP_INQUIRY ) );
}

// --

static S32
_pairing_AddController( struct Bluetooth2PairingIFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location )
{
	return ( RegisterController( (PTR) self->Data.LibBase, port, location, BT2_CAP_INQUIRY | BT2_CAP_PAIRING ) );
}

// --

static S32
_bonding_AddController( struct Bluetooth2BondingIFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location )
{
	return ( RegisterController( (PTR) self->Data.LibBase, port, location,
		BT2_CAP_INQUIRY | BT2_CAP_PAIRING | BT2_CAP_BONDING ) );
}

// --
