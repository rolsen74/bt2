
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_all.h"

// --

void
BT2_RemoveController( struct Bluetooth2Base *base, U32 unit, struct MsgPort *port )
{
	struct BTController *controller;
	struct BTController **slot;
	struct BTControllerProbe *probe;

	base->Exec->ObtainSemaphore( & base->ControllerLock );

	slot = & base->Controllers;

	while( ( *slot ) && ( (*slot)->Unit < unit ) )
	{
		slot = & (*slot)->Next;
	}

	controller = *slot;

	if ( ( ! controller ) || ( controller->Unit != unit ) || ( controller->Port != port ) )
	{
		base->Exec->ReleaseSemaphore( & base->ControllerLock );

		return;
	}

	*slot = controller->Next;

	base->Exec->ReleaseSemaphore( & base->ControllerLock );

	// The worker calls removal before freeing its port. No new probes can reach it.
	while(( probe = (PTR) base->Exec->GetMsg( port ) ))
	{
		probe->Result = BT_HCI_NO_CONTROLLER;

		base->Exec->ReplyMsg( & probe->Message );
	}

	base->Exec->FreeVec( controller );
}

// --
