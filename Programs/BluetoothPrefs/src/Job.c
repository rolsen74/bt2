
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <string.h>
#include "Prefs.h"

// --

int
Prefs_StartJob ( struct PrefsWindow * gui, int pair )
{
	struct BTPairRequest request;
	struct PeerRow *	 peer;
	enum BTHCIResult	 result;

	ULONG	 pin;
	size_t	 length;
	uint32_t index;

	if ( ( ! gui->HaveAdapter ) || ( ( pair ) && ( ! gui->Adapter.Ready ) ) || ( gui->Busy ) )
	{
		return ( FALSE );
	}

	memset ( &request, 0, sizeof ( request ) );

	if ( pair )
	{
		peer = Prefs_Selected ( gui );
		pin	 = 0;

		GetAttr ( STRINGA_TextVal, gui->Gadgets[GAD_PIN], &pin );

		length = pin ? strlen ( (const char *)pin ) : 0;

		if ( ( ! peer ) || ( ! length ) || ( length > BT_PIN_MAX ) )
		{
			Prefs_SetStatus ( gui, "Select a device and enter its PIN (1-16 characters)." );

			return ( FALSE );
		}

		memcpy ( request.Address, peer->Address, 6 );

		memcpy ( request.PIN, (const void *)pin, length );

		request.PINLength = length;

		RefreshSetGadgetAttrs ( (struct Gadget *)gui->Gadgets[GAD_PIN], gui->Window, NULL, STRINGA_TextVal, "", TAG_END );

		for ( index = 0; index < sizeof ( gui->PIN ); index++ )
		{
			( (volatile char *)gui->PIN )[index]	 = 0;
			( (volatile char *)gui->PINUndo )[index] = 0;
			( (volatile char *)gui->PINWork )[index] = 0;
		}
	}

	result = gui->Bluetooth->control_Submit ( pair ? BT2_MANAGED_PAIR : BT2_MANAGED_SCAN, gui->Registration,
											  pair ? &request : NULL, &gui->Job.ID );

	for ( index = 0; index < sizeof ( request.PIN ); index++ )
	{
		( (volatile uint8_t *)request.PIN )[index] = 0;
	}

	if ( result != BT_HCI_OK )
	{
		Prefs_ResultStatus ( gui, result, NULL );

		return ( FALSE );
	}

	gui->Job.Pair = pair;
	gui->Busy	  = TRUE;

	Prefs_SetStatus ( gui, pair ? "Pairing..." : "Scanning for Classic Bluetooth devices..." );

	Prefs_RefreshControls ( gui );

	return ( TRUE );
}

// --

void
Prefs_PollJob ( struct PrefsWindow * gui )
{
	struct BT2OperationStatus status;
	enum BTHCIResult		  result;

	if ( ! gui->Busy )
	{
		return;
	}

	memset ( &status, 0, sizeof ( status ) );

	result = gui->Bluetooth->control_Operation ( gui->Job.ID, &status, gui->Job.Nearby, PREFS_PEERS );

	if ( ( result == BT_HCI_OK ) && ( status.Pending ) )
	{
		return;
	}

	gui->Busy	= FALSE;
	gui->Job.ID = 0;

	if ( ! gui->Job.Pair )
	{
		gui->Reports = ( result == BT_HCI_OK && status.Result == BT_HCI_OK ) ? status.Count : 0;
	}

	if ( gui->Window )
	{
		Prefs_RebuildRows ( gui, TRUE );
	}

	Prefs_ResultStatus ( gui, result == BT_HCI_OK ? status.Result : result, &status.Error );
}

// --
