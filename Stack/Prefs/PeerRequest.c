
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_prefs.h"

// --

enum BTHCIResult
_main_prefs_PeerRequest ( struct Bluetooth2IFace * self, const uint8_t * local, struct BTInquiryResult * inquiry,
						  struct BTBondRequest * bond, struct BTHCIError * error )
{
	static const struct BTControllerProbe empty = { 0 };
	struct Bluetooth2Base *				  base;
	struct BTController *				  controller;
	struct BTControllerLocation			  location;
	struct BTControllerProbe			  probe;
	enum BTHCIResult					  result;

	if ( ( ! local ) || ( ! error ) || ( ( ! inquiry ) == ( ! bond ) ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	base = (PTR)self->Data.LibBase;

	base->Exec->ObtainSemaphore ( &base->ControllerLock );

	controller = BT2_PrefsFindAdapter ( base, local );

	if ( controller )
	{
		controller->References++;
	}

	base->Exec->ReleaseSemaphore ( &base->ControllerLock );

	if ( ! controller )
	{
		return ( BT_HCI_NO_CONTROLLER );
	}

	base->Exec->CopyMem ( &empty, &probe, sizeof ( probe ) );

	probe.Error	  = error;
	probe.Bond	  = bond;
	probe.Inquiry = inquiry;

	result = BT2_RequestForController ( base, controller->Unit, controller, &location, &probe );

	BT2_ReleaseController ( base, controller );

	return ( result );
}

// --
