
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_all.h"

// --

S32
BT2_MasterCall ( struct Bluetooth2Base * base, struct BT2MasterRequest * request )
{
	struct ExecIFace * exec;
	struct MsgPort *   master;
	struct MsgPort *   reply;

	exec = base->Exec;

	exec->ObtainSemaphore ( &base->ControllerLock );

	master = base->MasterPort;

	exec->ReleaseSemaphore ( &base->ControllerLock );

	if ( ! master )
	{
		return ( FALSE );
	}

	reply = exec->AllocSysObjectTags ( ASOT_PORT, TAG_END );

	if ( ! reply )
	{
		return ( FALSE );
	}

	request->Message.mn_ReplyPort = reply;
	request->Message.mn_Length	  = sizeof ( *request );

	exec->PutMsg ( master, &request->Message );

	exec->WaitPort ( reply );

	exec->GetMsg ( reply );

	exec->FreeSysObject ( ASOT_PORT, reply );

	return ( TRUE );
}

// --
