
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

static S32
_names_AddController( struct Bluetooth2NamesIFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location )
{
	return ( RegisterController( (PTR) self->Data.LibBase, port, location,
		BT2_CAP_INQUIRY | BT2_CAP_PAIRING | BT2_CAP_BONDING | BT2_CAP_NAMES ) );
}

// --

static S32
_visibility_AddController( struct Bluetooth2VisibilityIFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location )
{
	return ( RegisterController( (PTR) self->Data.LibBase, port, location,
		BT2_CAP_INQUIRY | BT2_CAP_PAIRING | BT2_CAP_BONDING | BT2_CAP_NAMES | BT2_CAP_VISIBILITY ) );
}

// --

static S32
_l2cap_AddController( struct Bluetooth2L2CAPIFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location )
{
	return ( RegisterController( (PTR) self->Data.LibBase, port, location,
		BT2_CAP_INQUIRY | BT2_CAP_PAIRING | BT2_CAP_BONDING | BT2_CAP_NAMES | BT2_CAP_VISIBILITY | BT2_CAP_L2CAP ) );
}

// --

static S32
_sdp_AddController( struct Bluetooth2SDPIFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location )
{
	return ( RegisterController( (PTR) self->Data.LibBase, port, location,
		BT2_CAP_INQUIRY | BT2_CAP_PAIRING | BT2_CAP_BONDING | BT2_CAP_NAMES |
		BT2_CAP_VISIBILITY | BT2_CAP_L2CAP | BT2_CAP_SDP ) );
}

// --

static S32
_rfcomm_AddController( struct Bluetooth2RFCOMMIFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location )
{
	return ( RegisterController( (PTR) self->Data.LibBase, port, location,
		BT2_CAP_INQUIRY | BT2_CAP_PAIRING | BT2_CAP_BONDING | BT2_CAP_NAMES |
		BT2_CAP_VISIBILITY | BT2_CAP_L2CAP | BT2_CAP_SDP | BT2_CAP_RFCOMM ) );
}

// --

static S32
_serial_AddController( struct Bluetooth2SerialIFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location )
{
	return ( RegisterController( (PTR) self->Data.LibBase, port, location,
		BT2_CAP_INQUIRY | BT2_CAP_PAIRING | BT2_CAP_BONDING | BT2_CAP_NAMES |
		BT2_CAP_VISIBILITY | BT2_CAP_L2CAP | BT2_CAP_SDP | BT2_CAP_RFCOMM | BT2_CAP_SERIAL ) );
}

// --

static S32
_serialfile_AddController( struct Bluetooth2SerialFileIFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location )
{
	return ( RegisterController( (PTR) self->Data.LibBase, port, location,
		BT2_CAP_INQUIRY | BT2_CAP_PAIRING | BT2_CAP_BONDING | BT2_CAP_NAMES |
		BT2_CAP_VISIBILITY | BT2_CAP_L2CAP | BT2_CAP_SDP | BT2_CAP_RFCOMM | BT2_CAP_SERIAL | BT2_CAP_SERIALFILE ) );
}

// --

static S32
_serialcapture_AddController( struct Bluetooth2SerialCaptureIFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location )
{
	return ( RegisterController( (PTR) self->Data.LibBase, port, location,
		BT2_CAP_INQUIRY | BT2_CAP_PAIRING | BT2_CAP_BONDING | BT2_CAP_NAMES | BT2_CAP_VISIBILITY |
		BT2_CAP_L2CAP | BT2_CAP_SDP | BT2_CAP_RFCOMM | BT2_CAP_SERIAL | BT2_CAP_SERIALFILE | BT2_CAP_SERIALCAPTURE ) );
}

// --

static S32
_objectpush_AddController( struct Bluetooth2ObjectPushIFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location )
{
	return ( RegisterController( (PTR) self->Data.LibBase, port, location,
		BT2_CAP_INQUIRY | BT2_CAP_PAIRING | BT2_CAP_BONDING | BT2_CAP_NAMES | BT2_CAP_VISIBILITY |
		BT2_CAP_L2CAP | BT2_CAP_SDP | BT2_CAP_RFCOMM | BT2_CAP_SERIAL | BT2_CAP_SERIALFILE |
		BT2_CAP_SERIALCAPTURE | BT2_CAP_OBJECTPUSH ) );
}

// --

static S32
_le_AddController( struct Bluetooth2LEIFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location, U32 acl )
{
	U32 capabilities;

	capabilities = BT2_CAP_LE | BT2_CAP_INQUIRY | BT2_CAP_PAIRING | BT2_CAP_BONDING |
		BT2_CAP_NAMES | BT2_CAP_VISIBILITY;

	if ( acl )
	{
		capabilities |= BT2_CAP_L2CAP | BT2_CAP_SDP | BT2_CAP_RFCOMM | BT2_CAP_SERIAL |
			BT2_CAP_SERIALFILE | BT2_CAP_SERIALCAPTURE | BT2_CAP_OBJECTPUSH;
	}

	return ( RegisterController( (PTR) self->Data.LibBase, port, location, capabilities ) );
}

// --
