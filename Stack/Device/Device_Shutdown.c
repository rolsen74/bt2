
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
ROMExit( struct Bluetooth2Base *base )
{
	struct ExecIFace *exec;

	PTR seglist;

	seglist = base->SegList;

	// Keep the resident installed while its master process is running.
	if ( ( ! seglist ) || ( base->MasterPort ) )
	{
		return ( NULL );
	}

	if ( base->Device.dd_Library.lib_OpenCnt )
	{
		base->Device.dd_Library.lib_Flags |= LIBF_DELEXP;

		return ( NULL );
	}

	exec = base->Exec;

	exec->Remove( & base->Device.dd_Library.lib_Node );

	exec->DeleteLibrary( & base->Device.dd_Library );

	return ( seglist );
}

// --
