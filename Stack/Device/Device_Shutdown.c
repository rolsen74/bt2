
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
ROMExit ( struct Bluetooth2Base * base )
{
	struct ExecIFace * exec;

	PTR seglist;

	seglist = base->SegList;

	// Keep the resident installed while its master process is running.
	if ( ( ! seglist ) || ( base->MasterPort ) || ( base->ShareState != BT_SHARE_STOPPED ) )
	{
		return ( NULL );
	}

	if ( base->Device.dd_Library.lib_OpenCnt )
	{
		base->Device.dd_Library.lib_Flags |= LIBF_DELEXP;

		return ( NULL );
	}

	exec = base->Exec;

	exec->Remove ( &base->Device.dd_Library.lib_Node );

	exec->DeleteLibrary ( &base->Device.dd_Library );

	return ( seglist );
}

// --
