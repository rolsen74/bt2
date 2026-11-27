
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/timer.h>
#include "bt2_device.h"

// --

int
DeadlineExpired ( PTR context )
{
	struct BTDeadline * timer;
	struct TimeVal		now;

	U32 seconds;

	timer = context;

	timer->Interface->GetUpTime ( &now );

	if ( ! timer->Started )
	{
		timer->Start   = now;
		timer->Started = TRUE;

		return ( FALSE );
	}

	seconds = now.Seconds - timer->Start.Seconds;

	return ( ( seconds > timer->Seconds )
			 || ( ( seconds == timer->Seconds ) && ( now.Microseconds >= timer->Start.Microseconds ) ) );
}

// --

enum BTHCIResult
OpenDeadline ( struct Bluetooth2Base * base, struct BTDeadline * timer, U32 seconds )
{
	enum BTHCIResult result;

	timer->Seconds = seconds;
	timer->Started = FALSE;

	timer->Port = base->Exec->AllocSysObjectTags ( ASOT_PORT, TAG_END );

	if ( ! timer->Port )
	{
		return ( BT_HCI_NO_MEMORY );
	}

	result = BT_HCI_NO_MEMORY;

	timer->Request = base->Exec->AllocSysObjectTags ( ASOT_IOREQUEST, ASOIOR_Size, sizeof ( *timer->Request ), ASOIOR_ReplyPort,
													  timer->Port, TAG_END );

	if ( timer->Request )
	{
		result = BT_HCI_TIMER_ERROR;

		if ( ! base->Exec->OpenDevice ( TIMERNAME, UNIT_MICROHZ, (PTR)timer->Request, 0 ) )
		{
			timer->Interface = (PTR)base->Exec->GetInterface ( (PTR)timer->Request->Request.io_Device, "main", 1, NULL );

			if ( timer->Interface )
			{
				return ( BT_HCI_OK );
			}

			base->Exec->CloseDevice ( (PTR)timer->Request );
		}

		base->Exec->FreeSysObject ( ASOT_IOREQUEST, timer->Request );
	}

	base->Exec->FreeSysObject ( ASOT_PORT, timer->Port );

	return ( result );
}

// --

void
CloseDeadline ( struct Bluetooth2Base * base, struct BTDeadline * timer )
{
	base->Exec->DropInterface ( (PTR)timer->Interface );

	base->Exec->CloseDevice ( (PTR)timer->Request );

	base->Exec->FreeSysObject ( ASOT_IOREQUEST, timer->Request );

	base->Exec->FreeSysObject ( ASOT_PORT, timer->Port );
}

// --
