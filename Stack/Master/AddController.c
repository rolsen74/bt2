
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_all.h"

// --

S32
BT2_AddController ( struct Bluetooth2Base * base, struct MsgPort * port, const struct BTControllerLocation * location,
					S32 capabilities )
{
	struct BTController *  controller;
	struct BTController ** slot;

	U32 unit;

	if ( ( ! port ) || ( ! location ) )
	{
		return ( -1 );
	}

	// Exec stores allocation flags alongside the message arrival action.
	if ( ( ( port->mp_Flags & PF_ACTION ) != PA_SIGNAL ) || ( ! port->mp_SigTask ) )
	{
		base->Exec->DebugPrintF ( "Bluetooth2: controller registration requires a task signal port (flags $%02lx)\n",
								  (U32)port->mp_Flags );

		return ( -1 );
	}

	controller = base->Exec->AllocVecTags ( sizeof ( *controller ), AVT_ClearWithValue, 0, TAG_END );

	if ( ! controller )
	{
		base->Exec->DebugPrintF ( "Bluetooth2: could not allocate controller registry entry\n" );

		return ( -1 );
	}

	controller->Base		 = base;
	controller->Port		 = port;
	controller->Worker		 = port->mp_SigTask;
	controller->Exclusive	 = NULL;
	controller->Location	 = *location;
	controller->Capabilities = capabilities;
	controller->References	 = 1;
	unit					 = 0;

	base->Exec->ObtainSemaphore ( &base->ControllerLock );

	slot = &base->Controllers;

	while ( ( *slot ) && ( ( *slot )->Unit == unit ) )
	{
		unit++;
		slot = &( *slot )->Next;
	}

	controller->Registration = ++base->NextRegistration;

	if ( ! controller->Registration )
	{
		controller->Registration = ++base->NextRegistration;
	}

	controller->Unit = unit;
	controller->Next = *slot;
	*slot			 = controller;

	base->Exec->ReleaseSemaphore ( &base->ControllerLock );

	return ( unit );
}

// --
