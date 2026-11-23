
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_all.h"

// --

S32
BT2_AddController( struct Bluetooth2Base *base, struct MsgPort *port,
	const struct BTControllerLocation *location, S32 capabilities )
{
	struct BTController *controller;
	struct BTController **slot;

	U32 unit;

	if ( ( ! port ) || ( ! location ) )
	{
		return ( -1 );
	}

	controller = base->Exec->AllocVecTags( sizeof( *controller ), TAG_END );

	if ( ! controller )
	{
		return ( -1 );
	}

	controller->Port = port;
	controller->Location = *location;
	controller->Capabilities = capabilities;
	controller->References = 1;
	unit = 0;

	base->Exec->ObtainSemaphore( & base->ControllerLock );

	slot = & base->Controllers;

	while( ( *slot ) && ( (*slot)->Unit == unit ) )
	{
		unit++;
		slot = & (*slot)->Next;
	}

	controller->Unit = unit;
	controller->Next = *slot;
	*slot = controller;

	base->Exec->ReleaseSemaphore( & base->ControllerLock );

	return ( unit );
}

// --
