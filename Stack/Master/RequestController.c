
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <stddef.h>
#include "bt2_all.h"

// --

static void
WaitForCancellableReply( struct Bluetooth2Base *base, struct MsgPort *reply, uint32_t *cancel )
{
	U32 signals;
	U32 mask;

	mask = ( 1UL << reply->mp_SigBit ) | SIGBREAKF_CTRL_C;

	while( ! base->Exec->GetMsg( reply ) )
	{
		signals = base->Exec->Wait( mask );

		if ( signals & SIGBREAKF_CTRL_C )
		{
			__atomic_store_n( cancel, 1, __ATOMIC_RELAXED );
		}
	}
}

// --

enum BTHCIResult
BT2_RequestController( struct Bluetooth2Base *base, U32 unit, struct BTControllerLocation *location,
	struct BTControllerProbe *probe )
{
	struct BTController *controller;
	struct MsgPort *reply;

	reply = base->Exec->AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! reply )
	{
		return ( BT_HCI_NO_MEMORY );
	}

	probe->Message.mn_ReplyPort = reply;
	probe->Message.mn_Length = offsetof( struct BTControllerProbe, Pair );

	if ( probe->Pair )
	{
		probe->Message.mn_Length = offsetof( struct BTControllerProbe, Bond );
	}
	else if ( probe->Bond )
	{
		probe->Message.mn_Length = offsetof( struct BTControllerProbe, RemoteName );
	}
	else if ( probe->RemoteName )
	{
		probe->Message.mn_Length = offsetof( struct BTControllerProbe, Visibility );
	}
	else if ( probe->Visibility )
	{
		probe->Message.mn_Length = offsetof( struct BTControllerProbe, EchoAddress );
	}
	else if ( probe->EchoAddress )
	{
		probe->Message.mn_Length = offsetof( struct BTControllerProbe, Services );
	}
	else if ( probe->Services )
	{
		probe->Message.mn_Length = offsetof( struct BTControllerProbe, RFCOMM );
	}
	else if ( probe->RFCOMM )
	{
		probe->Message.mn_Length = offsetof( struct BTControllerProbe, Serial );
	}
	else if ( probe->Serial )
	{
		probe->Message.mn_Length = offsetof( struct BTControllerProbe, SerialFile );
	}
	else if ( probe->SerialFile )
	{
		probe->Message.mn_Length = offsetof( struct BTControllerProbe, SerialCapture );
	}
	else if ( probe->SerialCapture )
	{
		probe->Message.mn_Length = offsetof( struct BTControllerProbe, ObjectPush );
	}
	else if ( probe->ObjectPush )
	{
		probe->Message.mn_Length = offsetof( struct BTControllerProbe, LE );
	}
	else if ( probe->LE )
	{
		probe->Message.mn_Length = offsetof( struct BTControllerProbe, Audio );
	}
	else if ( probe->Audio )
	{
		probe->Message.mn_Length = offsetof( struct BTControllerProbe, PCM );
	}
	else if ( probe->PCM )
	{
		probe->Message.mn_Length = offsetof( struct BTControllerProbe, HID );
	}
	else if ( probe->HID )
	{
		probe->Message.mn_Length = sizeof( *probe );
	}

	probe->Result = BT_HCI_NO_CONTROLLER;

	base->Exec->ObtainSemaphore( & base->ControllerLock );

	controller = base->Controllers;

	while( ( controller ) && ( controller->Unit < unit ) )
	{
		controller = controller->Next;
	}

	if ( ( ! controller ) || ( controller->Unit != unit ) )
	{
		base->Exec->ReleaseSemaphore( & base->ControllerLock );

		base->Exec->FreeSysObject( ASOT_PORT, reply );

		return ( BT_HCI_NO_CONTROLLER );
	}

	*location = controller->Location;

	if ( ( ( probe->Inquiry ) && ( ! ( controller->Capabilities & BT2_CAP_INQUIRY ) ) ) ||
		( ( probe->Pair ) && ( ! ( controller->Capabilities & BT2_CAP_PAIRING ) ) ) ||
		( ( probe->Bond ) && ( ! ( controller->Capabilities & BT2_CAP_BONDING ) ) ) ||
		( ( probe->RemoteName ) && ( ! ( controller->Capabilities & BT2_CAP_NAMES ) ) ) ||
		( ( probe->Visibility ) && ( ! ( controller->Capabilities & BT2_CAP_VISIBILITY ) ) ) ||
		( ( probe->EchoAddress ) && ( ! ( controller->Capabilities & BT2_CAP_L2CAP ) ) ) ||
		( ( probe->Services ) && ( ! ( controller->Capabilities & BT2_CAP_SDP ) ) ) ||
		( ( probe->RFCOMM ) && ( ! ( controller->Capabilities & BT2_CAP_RFCOMM ) ) ) ||
		( ( probe->Serial ) && ( ! ( controller->Capabilities & BT2_CAP_SERIAL ) ) ) ||
		( ( probe->SerialFile ) && ( ! ( controller->Capabilities & BT2_CAP_SERIALFILE ) ) ) ||
		( ( probe->SerialCapture ) && ( ! ( controller->Capabilities & BT2_CAP_SERIALCAPTURE ) ) ) ||
		( ( probe->ObjectPush ) && ( ! ( controller->Capabilities & BT2_CAP_OBJECTPUSH ) ) ) ||
		( ( probe->LE ) && ( ! ( controller->Capabilities & BT2_CAP_LE ) ) ) ||
		( ( probe->Audio ) && ( ! ( controller->Capabilities & BT2_CAP_A2DP ) ) ) ||
		( ( probe->PCM ) && ( ! ( controller->Capabilities & BT2_CAP_PCM ) ) ) ||
		( ( probe->HID ) && ( ! ( controller->Capabilities & ( probe->HID->Seconds ? BT2_CAP_HID : BT2_CAP_HID_INPUT ) ) ) ) )
	{
		base->Exec->ReleaseSemaphore( & base->ControllerLock );

		base->Exec->FreeSysObject( ASOT_PORT, reply );

		return ( BT_HCI_UNSUPPORTED );
	}

	base->Exec->PutMsg( controller->Port, & probe->Message );

	base->Exec->ReleaseSemaphore( & base->ControllerLock );

	if ( probe->SerialFile )
	{
		WaitForCancellableReply( base, reply, & probe->SerialFile->Cancel );
	}
	else if ( probe->SerialCapture )
	{
		WaitForCancellableReply( base, reply, & probe->SerialCapture->Cancel );
	}
	else if ( probe->ObjectPush )
	{
		WaitForCancellableReply( base, reply, & probe->ObjectPush->Cancel );
	}
	else if ( probe->LE )
	{
		WaitForCancellableReply( base, reply, & probe->LE->Cancel );
	}
	else if ( probe->Audio )
	{
		WaitForCancellableReply( base, reply, & probe->Audio->Cancel );
	}
	else if ( probe->PCM )
	{
		WaitForCancellableReply( base, reply, & probe->PCM->Audio.Cancel );
	}
	else if ( probe->HID )
	{
		WaitForCancellableReply( base, reply, & probe->HID->Cancel );
	}
	else
	{
		base->Exec->WaitPort( reply );

		base->Exec->GetMsg( reply );
	}

	base->Exec->FreeSysObject( ASOT_PORT, reply );

	return ( probe->Result );
}

// --
