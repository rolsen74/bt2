
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_all.h"

// --

#include "NSCmd_DeviceQuery.c"

// --

static void
_manager_BeginIO( struct DeviceManagerInterface *self, struct IORequest *ioreq )
{
	struct Bluetooth2Base *base;

	if ( ioreq->io_Command == NSCMD_DEVICEQUERY )
	{
		NSCmd_DeviceQuery( ioreq );
	}
	else
	{
		ioreq->io_Error = IOERR_NOCMD;
	}

	if ( ! ( ioreq->io_Flags & IOF_QUICK ) )
	{
		base = (PTR) self->Data.LibBase;
		ioreq->io_Message.mn_Node.ln_Type = NT_MESSAGE;

		base->Exec->ReplyMsg( & ioreq->io_Message );
	}
}

// --
