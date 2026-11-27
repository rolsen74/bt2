
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_share.h"

// --

enum BTHCIResult
_main_fileshare_Listen ( struct Bluetooth2IFace * self, struct BTFileShare * share, struct BTHCIError * error )
{
	struct Bluetooth2Base * base;
	struct BTController *	controller;
	enum BTHCIResult		result;

	if ( ( ! share ) || ( ! error ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	base = (PTR)self->Data.LibBase;

	base->Exec->ObtainSemaphore ( &base->ControllerLock );

	controller = BT2_PrefsFindAdapter ( base, share->Local );

	if ( controller )
	{
		controller->References++;
	}

	base->Exec->ReleaseSemaphore ( &base->ControllerLock );

	if ( ! controller )
	{
		return ( BT_HCI_NO_CONTROLLER );
	}

	result = BT2_RequestShare ( base, controller, share, error );

	BT2_ReleaseController ( base, controller );

	return ( result );
}

// --

enum BTHCIResult
BT2_RequestShare ( struct Bluetooth2Base * base, struct BTController * controller, struct BTFileShare * share,
				   struct BTHCIError * error )
{
	static const struct BTControllerProbe empty = { 0 };
	struct BTControllerLocation			  location;
	struct BTControllerProbe			  probe;

	base->Exec->CopyMem ( &empty, &probe, sizeof ( probe ) );

	probe.Share = share;
	probe.Error = error;

	return ( BT2_RequestForController ( base, controller->Unit, controller, &location, &probe ) );
}

// --
