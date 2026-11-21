
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_all.h"

// --

static PTR
_manager_Close( struct DeviceManagerInterface *self, struct IORequest *ioreq )
{
	struct Bluetooth2Base *base;
	struct ExecIFace *exec;

	PTR seglist;

	base = (PTR) self->Data.LibBase;
	exec = base->Exec;
	seglist = NULL;

	exec->Forbid();

	if ( ioreq->io_Unit == & base->Unit )
	{
		ioreq->io_Device = NULL;
		ioreq->io_Unit = NULL;
		base->Unit.unit_OpenCnt--;
		base->Device.dd_Library.lib_OpenCnt--;

		if ( base->Device.dd_Library.lib_Flags & LIBF_DELEXP )
		{
			seglist = ROMExit( base );
		}
	}

	exec->Permit();

	return ( seglist );
}

// --
