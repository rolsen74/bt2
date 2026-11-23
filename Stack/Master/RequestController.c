
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

static U32 *
CancelFlag( const struct BTControllerProbe *probe )
{
	if ( probe->SerialFile )
	{
		return ( & probe->SerialFile->Cancel );
	}
	if ( probe->SerialCapture )
	{
		return ( & probe->SerialCapture->Cancel );
	}
	if ( probe->ObjectPush )
	{
		return ( & probe->ObjectPush->Cancel );
	}
	if ( probe->LE )
	{
		return ( & probe->LE->Cancel );
	}
	if ( probe->Audio )
	{
		return ( & probe->Audio->Cancel );
	}
	if ( probe->PCM )
	{
		return ( & probe->PCM->Audio.Cancel );
	}
	if ( probe->HID )
	{
		return ( & probe->HID->Cancel );
	}

	return ( NULL );
}

// --

static S32
CancelQueued( struct Bluetooth2Base *base, struct BTController *controller, struct BTControllerProbe *probe )
{
	struct MsgPort *port;
	struct Node *node;

	S32 removed;

	removed = FALSE;

	base->Exec->ObtainSemaphore( & base->ControllerLock );

	port = controller->Port;

	if ( port )
	{
		// This task-signalled port has no interrupt producer. Exclude
		// the worker's GetMsg while checking whether it claimed the probe.
		base->Exec->Forbid();

		for( node = port->mp_MsgList.lh_Head; node->ln_Succ; node = node->ln_Succ )
		{
			if ( node == & probe->Message.mn_Node )
			{
				base->Exec->Remove( node );

				removed = TRUE;

				break;
			}
		}

		base->Exec->Permit();

		if ( ! removed )
		{
			base->Exec->Signal( controller->Worker, 1UL << port->mp_SigBit );
		}
	}

	base->Exec->ReleaseSemaphore( & base->ControllerLock );

	return ( removed );
}

// --

static void
WaitForCancellableReply( struct Bluetooth2Base *base, struct BTController *controller,
	struct BTControllerProbe *probe, U32 *cancel )
{
	struct MsgPort *reply;

	U32 signals;
	U32 mask;

	reply = probe->Message.mn_ReplyPort;
	mask = ( 1UL << reply->mp_SigBit ) | SIGBREAKF_CTRL_C;

	while( ! base->Exec->GetMsg( reply ) )
	{
		signals = base->Exec->Wait( mask );

		if ( signals & SIGBREAKF_CTRL_C )
		{
			__atomic_store_n( cancel, 1, __ATOMIC_RELAXED );

			if ( CancelQueued( base, controller, probe ) )
			{
				probe->Result = probe->LE ? BT_LE_CANCELLED : BT_RFCOMM_CANCELLED;

				return;
			}
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
	struct Task *caller;

	U32 *cancel;

	if ( ( ! location ) || ( ! probe->Error ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	caller = base->Exec->FindTask( NULL );

	cancel = CancelFlag( probe );

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

	if ( ( controller->Worker == caller ) || ( controller->Exclusive ) ||
		( ( probe->PCM ) && ( controller->References > 1 ) ) )
	{
		base->Exec->ReleaseSemaphore( & base->ControllerLock );

		base->Exec->FreeSysObject( ASOT_PORT, reply );

		return ( BT_HCI_CONTROLLER_BUSY );
	}

	if ( ( probe->PCM ) || ( probe->Audio ) || ( probe->SerialCapture ) ||
		( ( probe->HID ) && ( ! probe->HID->Seconds ) ) )
	{
		controller->Exclusive = probe;
	}

	controller->References++;

	base->Exec->PutMsg( controller->Port, & probe->Message );

	base->Exec->ReleaseSemaphore( & base->ControllerLock );

	if ( cancel )
	{
		WaitForCancellableReply( base, controller, probe, cancel );
	}
	else
	{
		base->Exec->WaitPort( reply );

		base->Exec->GetMsg( reply );
	}

	base->Exec->ObtainSemaphore( & base->ControllerLock );

	if ( controller->Exclusive == probe )
	{
		controller->Exclusive = NULL;
	}

	base->Exec->ReleaseSemaphore( & base->ControllerLock );

	BT2_ReleaseController( base, controller );

	base->Exec->FreeSysObject( ASOT_PORT, reply );

	return ( probe->Result );
}

// --
