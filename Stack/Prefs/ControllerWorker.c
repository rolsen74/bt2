
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/dos.h>
#include <dos/dostags.h>
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
ReadAddress ( struct BTController * controller )
{
	static const struct BTControllerProbe empty = { 0 };
	struct BTControllerInfo				  info;
	struct BTControllerLocation			  location;
	struct BTControllerProbe			  probe;
	struct BTHCIError					  error;
	struct Bluetooth2Base *				  base;
	enum BTHCIResult					  result;

	base = controller->Base;

	base->Exec->CopyMem ( &empty, &probe, sizeof ( probe ) );

	probe.Info	= &info;
	probe.Error = &error;

	error.TransportError = 0;

	result = BT2_RequestForController ( base, controller->Unit, controller, &location, &probe );

	base->Exec->ObtainSemaphore ( &base->ControllerLock );

	if ( result == BT_HCI_OK )
	{
		base->Exec->CopyMem ( info.Address, controller->Address, sizeof ( info.Address ) );

		controller->Ready = TRUE;
	}
	else if ( ( result != BT_HCI_CONTROLLER_BUSY ) && ( result != BT_HCI_NO_CONTROLLER )
			  && ( result != controller->LastResult ) )
	{
		base->Exec->DebugPrintF ( "Bluetooth Prefs: unit %lu address probe failed (result %lu, transport %ld)\n",
								  controller->Unit, (U32)result, error.TransportError );
	}

	controller->LastResult = result;

	base->Exec->ReleaseSemaphore ( &base->ControllerLock );

	return ( result == BT_HCI_OK );
}

// --

void
BT2_PrefsInputState ( struct BTController * controller, enum BT2PrefsState state )
{
	struct Bluetooth2Base * base;

	base = controller->Base;

	base->Exec->ObtainSemaphore ( &base->ControllerLock );

	if ( controller->InputState != BT2_PREFS_DISCONNECTING )
	{
		controller->InputState = state;
	}

	base->Exec->ReleaseSemaphore ( &base->ControllerLock );
}

// --

static S32
SelectPeer ( struct BTController * controller, uint8_t * peer )
{
	struct Bluetooth2Base * base;

	S32 connect;

	base = controller->Base;

	base->Exec->ObtainSemaphore ( &base->ControllerLock );

	if ( ( controller->ConnectRequested ) && ( controller->InputAutomatic )
		 && ( controller->InputRevision != base->PolicyRevision ) )
	{
		controller->ConnectRequested = FALSE;
	}

	connect = ( controller->Port ) && ( controller->Capabilities & BT2_CAP_HID_INPUT ) && ( ! controller->Exclusive )
			  && ( ! controller->Pending ) && ( controller->ConnectRequested );

	if ( connect )
	{
		base->Exec->SetSignal ( 0, SIGBREAKF_CTRL_C );

		controller->ConnectRequested = FALSE;
		controller->InputState		 = BT2_PREFS_CONNECTING;

		base->Exec->CopyMem ( controller->Peer, peer, sizeof ( controller->Peer ) );
	}

	base->Exec->ReleaseSemaphore ( &base->ControllerLock );

	return ( connect );
}

// --

static S32
ControllerPrefsWorker ( void )
{
	struct ExecIFace *		exec;
	struct Bluetooth2Base * base;
	struct BTController *	controller;
	struct Task *			task;
	struct Library *		library;
	struct DOSIFace *		dos;
	enum BTHCIResult		result;

	uint8_t peer[6];
	S32		present;
	S32		ready;
	U32		probeDelay;

	exec = ExecInterface ();

	task = exec->FindTask ( NULL );

	controller = task->tc_UserData;
	base	   = controller->Base;

	library = exec->OpenLibrary ( "dos.library", 50 );

	if ( ! library )
	{
		goto done;
	}

	dos = (PTR)exec->GetInterface ( library, "main", 1, NULL );

	if ( ! dos )
	{
		goto close;
	}

	exec->ObtainSemaphore ( &base->ControllerLock );

	controller->PrefsWorker = task;

	exec->ReleaseSemaphore ( &base->ControllerLock );

	ready	   = FALSE;
	probeDelay = 0;

	for ( ;; )
	{
		exec->ObtainSemaphore ( &base->ControllerLock );

		present = controller->Port != NULL;

		exec->ReleaseSemaphore ( &base->ControllerLock );

		if ( ! present )
		{
			break;
		}

		if ( ( ! ready ) && ( ! probeDelay ) )
		{
			ready	   = ReadAddress ( controller );
			probeDelay = 5;
		}

		if ( ( ready ) && ( SelectPeer ( controller, peer ) ) )
		{
			result = BT2_PrefsRunInput ( controller, peer );

			exec->ObtainSemaphore ( &base->ControllerLock );

			controller->InputState = BT2_PREFS_IDLE;
			controller->LastResult = result;

			if ( result == BT_HCI_NO_SAVED_KEY )
			{
				controller->Paused = TRUE;
			}

			exec->ReleaseSemaphore ( &base->ControllerLock );
		}

		dos->Delay ( 50 );

		if ( probeDelay )
		{
			probeDelay--;
		}
	}

	exec->ObtainSemaphore ( &base->ControllerLock );

	controller->PrefsWorker = NULL;

	exec->ReleaseSemaphore ( &base->ControllerLock );

	exec->DropInterface ( (PTR)dos );

close:
	exec->CloseLibrary ( library );

done:
	exec->ObtainSemaphore ( &base->ControllerLock );

	controller->PrefsStarting = FALSE;

	exec->ReleaseSemaphore ( &base->ControllerLock );

	BT2_ReleaseController ( base, controller );

	return ( RETURN_OK );
}

// --

void
BT2_StartPrefsWorker ( struct Bluetooth2Base * base, U32 unit, struct MsgPort * port )
{
	struct BTController * controller;
	struct Library *	  library;
	struct DOSIFace *	  dos;
	struct Process *	  process;

	base->Exec->ObtainSemaphore ( &base->ControllerLock );

	for ( controller = base->Controllers; controller; controller = controller->Next )
	{
		if ( ( controller->Unit == unit ) && ( controller->Port == port ) && ( ! controller->PrefsStarting )
			 && ( ! controller->PrefsWorker ) )
		{
			controller->PrefsStarting = TRUE;
			controller->References++;

			break;
		}
	}

	base->Exec->ReleaseSemaphore ( &base->ControllerLock );

	if ( ! controller )
	{
		return;
	}

	process = NULL;

	library = base->Exec->OpenLibrary ( "dos.library", 50 );

	if ( library )
	{
		dos = (PTR)base->Exec->GetInterface ( library, "main", 1, NULL );

		if ( dos )
		{
			process = dos->CreateNewProcTags ( NP_Name, "Bluetooth input", NP_Entry, ControllerPrefsWorker, NP_UserData,
											   controller, NP_Child, FALSE, NP_StackSize, 32768, TAG_END );

			base->Exec->DropInterface ( (PTR)dos );
		}

		base->Exec->CloseLibrary ( library );
	}

	if ( ! process )
	{
		base->Exec->DebugPrintF ( "Bluetooth Prefs: could not start address and input worker for unit %lu\n", unit );

		base->Exec->ObtainSemaphore ( &base->ControllerLock );

		controller->PrefsStarting = FALSE;

		base->Exec->ReleaseSemaphore ( &base->ControllerLock );

		BT2_ReleaseController ( base, controller );
	}
}

// --
