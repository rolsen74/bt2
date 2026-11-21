
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_all.h"

// --

static void
_manager_Open( struct DeviceManagerInterface *self, struct IORequest *ioreq,
	U32 unit, U32 flags __attribute__((unused)) )
{
	struct Bluetooth2Base *base;

	if ( ( unit ) || ( ioreq->io_Message.mn_Length < sizeof( *ioreq ) ) )
	{
		ioreq->io_Device = NULL;
		ioreq->io_Error = IOERR_OPENFAIL;

		return;
	}

	base = (PTR) self->Data.LibBase;

	base->Exec->Forbid();

	base->Unit.unit_OpenCnt++;
	base->Device.dd_Library.lib_OpenCnt++;
	base->Device.dd_Library.lib_Flags &= ~LIBF_DELEXP;
	ioreq->io_Device = & base->Device;
	ioreq->io_Unit = & base->Unit;
	ioreq->io_Message.mn_Node.ln_Type = NT_REPLYMSG;
	ioreq->io_Error = 0;

	base->Exec->Permit();
}

// --
