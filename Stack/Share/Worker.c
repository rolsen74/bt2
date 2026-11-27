
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/dos.h>
#include <dos/dostags.h>
#include "bt2_share.h"
#include "bt2_control.h"

// --

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
static struct ExecIFace *
ExecInterface ( void )
{
	return ( (PTR)( *(struct ExecBase **)4 )->MainInterface );
}
#pragma GCC diagnostic pop

// --

static S32
ShareWorker ( void )
{
	struct ExecIFace *		exec;
	struct Task *			task;
	struct Bluetooth2Base * base;
	struct BTController *	controller;
	struct BTHCIError		error;
	enum BTHCIResult		result;

	exec = ExecInterface ();

	task = exec->FindTask ( NULL );

	base   = task->tc_UserData;
	error  = (struct BTHCIError){ 0 };
	result = BT_RFCOMM_CANCELLED;

	exec->ObtainSemaphore ( &base->ControllerLock );

	controller		= base->ShareController;
	base->ShareTask = task;

	exec->ReleaseSemaphore ( &base->ControllerLock );

	if ( ! __atomic_load_n ( &base->Share.Cancel, __ATOMIC_RELAXED ) )
	{
		result = BT2_RequestShare ( base, controller, &base->Share, &error );
	}

	BT2_ReleaseController ( base, controller );

	exec->ObtainSemaphore ( &base->ControllerLock );

	base->ShareController = NULL;
	base->ShareTask		  = NULL;
	base->ShareState	  = BT_SHARE_STOPPED;
	base->ShareResult	  = result;
	base->ShareError	  = error;

	exec->ReleaseSemaphore ( &base->ControllerLock );

	return ( RETURN_OK );
}

// --

enum BTHCIResult
BT2_RunShareForAddress ( struct Bluetooth2Base * base, const uint8_t * local, const char * path, S32 automatic, U32 revision )
{
	struct BTController * controller;
	struct Library *	  library;
	struct DOSIFace *	  dos;
	struct Process *	  process;
	enum BTHCIResult	  result;

	U32 length;
	U32 index;
	S32 absolute;

	if ( ( ! local ) || ( ! path ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	absolute = FALSE;

	for ( length = 0; ( length < BT_SHARE_PATH_SIZE ) && ( path[length] ); length++ )
	{
		if ( ( length ) && ( path[length] == ':' ) )
		{
			absolute = TRUE;
		}
	}

	if ( ( ! absolute ) || ( length >= BT_SHARE_PATH_SIZE ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	base->Exec->ObtainSemaphore ( &base->ControllerLock );

	controller = BT2_PrefsFindAdapter ( base, local );

	if ( ( automatic ) && ( ! base->ShareStartupArmed || base->ShareSuppressed || ( base->PolicyRevision != revision ) ) )
	{
		result = BT_RFCOMM_CANCELLED;
	}
	else if ( base->ShareState != BT_SHARE_STOPPED )
	{
		result = BT_HCI_CONTROLLER_BUSY;
	}
	else if ( ! controller )
	{
		result = BT_HCI_NO_CONTROLLER;
	}
	else if ( ! ( controller->Capabilities & BT2_CAP_SHARE ) )
	{
		result = BT_HCI_UNSUPPORTED;
	}
	else if ( ( controller->Exclusive ) || ( controller->ConnectRequested ) || ( controller->InputState != BT2_PREFS_IDLE ) )
	{
		result = BT_HCI_CONTROLLER_BUSY;
	}
	else
	{
		if ( ! automatic )
		{
			base->ShareSuppressed = FALSE;
		}

		controller->References++;
		base->ShareController	   = controller;
		base->ShareState		   = BT_SHARE_STARTING;
		base->ShareAutomatic	   = automatic;
		base->ShareAttemptRevision = revision;
		base->Share.Cancel		   = 0;
		base->Share.Listening	   = 0;
		base->ShareResult		   = BT_HCI_OK;
		base->ShareError		   = (struct BTHCIError){ 0 };
		result					   = BT_HCI_OK;

		base->Exec->CopyMem ( local, base->Share.Local, sizeof ( base->Share.Local ) );

		base->Exec->CopyMem ( path, base->Share.Path, length + 1 );

		for ( index = length + 1; index < BT_SHARE_PATH_SIZE; index++ )
		{
			base->Share.Path[index] = 0;
		}
	}

	base->Exec->ReleaseSemaphore ( &base->ControllerLock );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	process = NULL;

	library = base->Exec->OpenLibrary ( "dos.library", 50 );

	if ( library )
	{
		dos = (PTR)base->Exec->GetInterface ( library, "main", 1, NULL );

		if ( dos )
		{
			process = dos->CreateNewProcTags ( NP_Name, "Bluetooth file sharing", NP_Entry, ShareWorker, NP_UserData, base,
											   NP_Child, FALSE, NP_StackSize, 32768, NP_CurrentDir, 0, NP_ProgramDir, 0,
											   NP_ConsolePort, NULL, NP_WindowPtr, (PTR)-1, TAG_END );

			base->Exec->DropInterface ( (PTR)dos );
		}

		base->Exec->CloseLibrary ( library );
	}

	if ( process )
	{
		return ( BT_HCI_OK );
	}

	BT2_ReleaseController ( base, controller );

	base->Exec->ObtainSemaphore ( &base->ControllerLock );

	base->ShareController = NULL;
	base->ShareState	  = BT_SHARE_STOPPED;
	base->ShareResult	  = BT_HCI_NO_MEMORY;

	base->Exec->ReleaseSemaphore ( &base->ControllerLock );

	return ( BT_HCI_NO_MEMORY );
}

// --

enum BTHCIResult
_main_sharecontrol_Start ( struct Bluetooth2IFace * self, const uint8_t * local, const char * path )
{
	return ( BT2_RunShareForAddress ( (PTR)self->Data.LibBase, local, path, FALSE, 0 ) );
}

// --

void
BT2_StopManagedShare ( struct Bluetooth2Base * base )
{
	base->Exec->ObtainSemaphore ( &base->ControllerLock );

	base->ShareSuppressed = TRUE;

	if ( ( base->ShareState != BT_SHARE_STOPPED ) && ( base->ShareState != BT_SHARE_STOPPING ) )
	{
		base->ShareState = BT_SHARE_STOPPING;

		__atomic_store_n ( &base->Share.Cancel, 1, __ATOMIC_RELAXED );

		if ( base->ShareTask )
		{
			base->Exec->Signal ( base->ShareTask, SIGBREAKF_CTRL_C );
		}
	}

	base->Exec->ReleaseSemaphore ( &base->ControllerLock );
}

// --

void
_main_sharecontrol_Stop ( struct Bluetooth2IFace * self )
{
	BT2_StopManagedShare ( (PTR)self->Data.LibBase );
}

// --

void
_main_sharecontrol_Status ( struct Bluetooth2IFace * self, struct BTShareStatus * status )
{
	struct Bluetooth2Base * base;

	static const uint8_t empty[6] = { 0 };

	if ( ! status )
	{
		return;
	}

	base = (PTR)self->Data.LibBase;

	base->Exec->ObtainSemaphore ( &base->ControllerLock );

	status->State  = base->ShareState;
	status->Result = base->ShareResult;
	status->Error  = base->ShareError;

	if ( ( status->State == BT_SHARE_STARTING ) && ( __atomic_load_n ( &base->Share.Listening, __ATOMIC_RELAXED ) ) )
	{
		status->State = BT_SHARE_LISTENING;
	}

	if ( status->State != BT_SHARE_STOPPED )
	{
		base->Exec->CopyMem ( base->Share.Local, status->Local, sizeof ( status->Local ) );

		base->Exec->CopyMem ( base->Share.Path, status->Path, sizeof ( status->Path ) );
	}
	else
	{
		base->Exec->CopyMem ( empty, status->Local, sizeof ( status->Local ) );

		status->Path[0] = 0;
	}

	base->Exec->ReleaseSemaphore ( &base->ControllerLock );
}

// --
