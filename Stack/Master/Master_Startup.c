
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/dos.h>
#include <dos/dostags.h>
#include "bt2_prefs.h"

// --

struct MasterStartup
{
	struct Message			Message;
	struct Bluetooth2Base * Base;
	struct MsgPort *		Port;
};

// --

/* ExecBase lives at absolute address 4 in the AmigaOS ABI. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
static struct ExecIFace *
GetExecInterface ( void )
{
	return ( (PTR)( *(struct ExecBase **)4 )->MainInterface );
}
#pragma GCC diagnostic pop

// --

static S32
MasterEntry ( void )
{
	struct ExecIFace *		  exec;
	struct Task *			  task;
	struct MasterStartup *	  startup;
	struct Bluetooth2Base *	  base;
	struct MsgPort *		  port;
	struct BT2MasterRequest * request;
	struct MsgPort *		  completion;
	struct Message *		  message;

	U32 mask;

	exec = GetExecInterface ();

	task = exec->FindTask ( NULL );

	startup = task->tc_UserData;
	base	= startup->Base;

	port = exec->AllocSysObjectTags ( ASOT_PORT, TAG_END );

	completion = exec->AllocSysObjectTags ( ASOT_PORT, TAG_END );

	if ( ( ! port ) || ( ! completion ) )
	{
		if ( port )
		{
			exec->FreeSysObject ( ASOT_PORT, port );
		}

		if ( completion )
		{
			exec->FreeSysObject ( ASOT_PORT, completion );
		}

		exec->ReplyMsg ( &startup->Message );

		return ( RETURN_FAIL );
	}

	exec->ObtainSemaphore ( &base->ControllerLock );

	base->MasterPort	 = port;
	base->CompletionPort = completion;

	exec->ReleaseSemaphore ( &base->ControllerLock );

	startup->Port	  = port;
	task->tc_UserData = NULL;
	mask			  = ( 1UL << port->mp_SigBit ) | ( 1UL << completion->mp_SigBit );

	exec->ReplyMsg ( &startup->Message );

	BT2_StartPolicy ( base );

	for ( ;; )
	{
		exec->Wait ( mask );

		for ( ;; )
		{
			while ( ( message = exec->GetMsg ( completion ) ) )
			{
				BT2_ControlComplete ( base, message );
			}

			request = (PTR)exec->GetMsg ( port );

			if ( ! request )
			{
				break;
			}

			switch ( request->Operation )
			{
				case BT2_MASTER_ADD:
					request->Result = BT2_AddController ( base, request->Port, request->Location, request->Capabilities );
					break;

				case BT2_MASTER_REMOVE:
					BT2_RemoveController ( base, request->Unit, request->Port );
					break;

				case BT2_MASTER_QUEUE:
					request->Result = BT2_QueueProbe ( base, request );
					break;

				case BT2_MASTER_CONTROL:
					if ( BT2_ForwardControl ( base, request ) )
					{
						continue;
					}

					BT2_ControlDispatch ( base, request->Control );
					break;
			}

			exec->ReplyMsg ( &request->Message );
		}
	}
}

// --

S32
BT2_MasterStart ( struct Bluetooth2Base * base )
{
	struct ExecIFace *	 exec;
	struct DOSIFace *	 dos;
	struct Library *	 dosBase;
	struct Process *	 process;
	struct MsgPort *	 reply;
	struct MasterStartup startup;

	S32 started;

	if ( base->MasterPort )
	{
		return ( TRUE );
	}

	exec	= base->Exec;
	started = FALSE;

	dosBase = exec->OpenLibrary ( "dos.library", 50 );

	if ( ! dosBase )
	{
		return ( FALSE );
	}

	dos = (PTR)exec->GetInterface ( dosBase, "main", 1, NULL );

	if ( ! dos )
	{
		goto close_dos;
	}

	reply = exec->AllocSysObjectTags ( ASOT_PORT, TAG_END );

	if ( ! reply )
	{
		goto drop_dos;
	}

	startup.Message.mn_ReplyPort = reply;
	startup.Message.mn_Length	 = sizeof ( startup );
	startup.Base				 = base;
	startup.Port				 = NULL;

	process = dos->CreateNewProcTags ( NP_Name, "Bluetooth", NP_Child, FALSE, NP_Entry, MasterEntry, NP_UserData, &startup,
									   TAG_END );

	if ( process )
	{
		exec->WaitPort ( reply );

		exec->GetMsg ( reply );

		started = startup.Port != NULL;
	}

	exec->FreeSysObject ( ASOT_PORT, reply );

drop_dos:
	exec->DropInterface ( (PTR)dos );

close_dos:
	exec->CloseLibrary ( dosBase );

	return ( started );
}

// --
