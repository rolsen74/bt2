
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_all.h"

// --

void
BT2_ReleaseController ( struct Bluetooth2Base * base, struct BTController * controller )
{
	base->Exec->ObtainSemaphore ( &base->ControllerLock );

	controller->References--;

	if ( ! controller->References )
	{
		base->Exec->FreeVec ( controller );
	}

	base->Exec->ReleaseSemaphore ( &base->ControllerLock );
}

// --
