
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
		( ( probe->Bond ) && ( ! ( controller->Capabilities & BT2_CAP_BONDING ) ) ) )
	{
		base->Exec->ReleaseSemaphore( & base->ControllerLock );

		base->Exec->FreeSysObject( ASOT_PORT, reply );

		return ( BT_HCI_UNSUPPORTED );
	}

	base->Exec->PutMsg( controller->Port, & probe->Message );

	base->Exec->ReleaseSemaphore( & base->ControllerLock );

	base->Exec->WaitPort( reply );

	base->Exec->GetMsg( reply );

	base->Exec->FreeSysObject( ASOT_PORT, reply );

	return ( probe->Result );
}

// --
