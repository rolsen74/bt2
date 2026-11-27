
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_device.h"

// --

PTR
_manager_Expunge ( struct DeviceManagerInterface * self )
{
	struct Bluetooth2Base * base;
	struct ExecIFace *		exec;

	PTR seglist;

	base = (PTR)self->Data.LibBase;
	exec = base->Exec;

	exec->Forbid ();

	seglist = ROMExit ( base );

	exec->Permit ();

	return ( seglist );
}

// --
