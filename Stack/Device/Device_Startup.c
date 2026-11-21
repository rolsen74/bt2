
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
ROMInit( PTR dummy __attribute__((unused)), PTR seglist, struct ExecBase *sysbase )
{
	struct ExecIFace *exec;
	struct Bluetooth2Base *base;

	exec = (PTR) sysbase->MainInterface;

	if ( exec->FindName( & sysbase->DeviceList, BLUETOOTH2_NAME ) )
	{
		return ( NULL );
	}

	base = (PTR) exec->CreateLibraryTags( CLT_DataSize, sizeof( *base ),
		CLT_Interfaces, DeviceInterfaces,
		CLT_NoLegacyIFace, TRUE,
		TAG_END );

	if ( ! base )
	{
		return ( NULL );
	}

	base->Exec = exec;
	base->SegList = seglist;
	base->Controllers = NULL;
	base->MasterPort = NULL;
	base->Unit.unit_OpenCnt = 0;
	base->Device.dd_Library.lib_Node.ln_Type = NT_DEVICE;
	base->Device.dd_Library.lib_Node.ln_Name = BLUETOOTH2_NAME;
	base->Device.dd_Library.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
	base->Device.dd_Library.lib_Version = BLUETOOTH2_VERSION;
	base->Device.dd_Library.lib_Revision = BLUETOOTH2_REVISION;
	base->Device.dd_Library.lib_IdString = BLUETOOTH2_ID;

	exec->InitSemaphore( & base->ControllerLock );

	exec->InitSemaphore( & base->BondLock );

	exec->AddDevice( & base->Device );

	return ( NULL );
}

// --
