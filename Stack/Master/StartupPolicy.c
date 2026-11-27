
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/dos.h>
#include <dos/dostags.h>
#include <devices/timer.h>
#include "bt2_control.h"

// --

struct BT2StoppedInput
{
	struct BT2StoppedInput * Next;
	uint8_t					 Local[6];
};

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
SameAddress ( const uint8_t * left, const uint8_t * right )
{
	U32 index;

	for ( index = 0; index < 6; index++ )
	{
		if ( left[index] != right[index] )
		{
			return ( FALSE );
		}
	}

	return ( TRUE );
}

// --
// ControllerLock protects the boot-local stop list.
S32
BT2_SetInputStopped ( struct Bluetooth2Base * base, const uint8_t * local, S32 stop )
{
	struct BT2StoppedInput ** link;
	struct BT2StoppedInput *  entry;

	for ( link = &base->StoppedInputs; *link; link = &( *link )->Next )
	{
		if ( SameAddress ( ( *link )->Local, local ) )
		{
			break;
		}
	}

	entry = *link;

	if ( ( entry ) && ( ! stop ) )
	{
		*link = entry->Next;

		base->Exec->FreeVec ( entry );
	}
	else if ( ( ! entry ) && ( stop ) )
	{
		entry = base->Exec->AllocVecTags ( sizeof ( *entry ), TAG_END );

		if ( ! entry )
		{
			return ( FALSE );
		}

		base->Exec->CopyMem ( local, entry->Local, sizeof ( entry->Local ) );

		entry->Next = NULL;
		*link		= entry;
	}

	return ( TRUE );
}

// --

static S32
InputStopped ( struct Bluetooth2Base * base, const uint8_t * local )
{
	struct BT2StoppedInput * entry;

	for ( entry = base->StoppedInputs; entry; entry = entry->Next )
	{
		if ( SameAddress ( entry->Local, local ) )
		{
			return ( TRUE );
		}
	}

	return ( FALSE );
}

// --

static void
QueueBootInput ( struct Bluetooth2Base * base, struct BTController * controller, const uint8_t * peer )
{
	base->Exec->ObtainSemaphore ( &base->ControllerLock );

	if ( ( controller->Port ) && ( controller->Ready ) && ( controller->PrefsWorker )
		 && ( controller->Capabilities & BT2_CAP_HID_INPUT ) && ( ! controller->Paused ) && ( ! controller->Exclusive )
		 && ( ! controller->Pending ) && ( ! controller->ConnectRequested ) && ( controller->InputState == BT2_PREFS_IDLE )
		 && ( ! InputStopped ( base, controller->Address ) ) )
	{
		base->Exec->CopyMem ( peer, controller->Peer, sizeof ( controller->Peer ) );

		controller->InputAutomatic	 = TRUE;
		controller->InputRevision	 = base->PolicyRevision;
		controller->ConnectRequested = TRUE;
	}

	base->Exec->ReleaseSemaphore ( &base->ControllerLock );
}

// --

static void
ApplyInputPolicy ( struct Bluetooth2Base * base, struct DOSIFace * dos, const struct BT2StartupShare * sharing, S32 retry )
{
	struct BTController * controller;
	struct MsgPort *	  port;

	uint8_t peer[6];
	U32		next;
	U32		unit;
	S32		ready;
	S32		worker;

	next = 0;

	for ( ;; )
	{
		base->Exec->ObtainSemaphore ( &base->ControllerLock );

		for ( controller = base->Controllers; controller && ( controller->Unit < next ); controller = controller->Next )
		{
		}

		if ( ! controller )
		{
			base->Exec->ReleaseSemaphore ( &base->ControllerLock );

			return;
		}

		controller->References++;
		unit   = controller->Unit;
		next   = unit + 1;
		port   = controller->Port;
		worker = controller->PrefsStarting || controller->PrefsWorker;
		ready  = controller->Ready;

		base->Exec->ReleaseSemaphore ( &base->ControllerLock );

		if ( ! worker )
		{
			BT2_StartPrefsWorker ( base, unit, port );
		}

		if ( ( ready ) && ( retry ) )
		{
			base->Exec->ObtainSemaphore ( &base->BondLock );

			if ( BT2_ReadAutoPeer ( dos, controller->Address, peer )
				 && ( ! sharing->Enabled || ! SameAddress ( sharing->Local, controller->Address ) ) )
			{
				QueueBootInput ( base, controller, peer );
			}

			base->Exec->ReleaseSemaphore ( &base->BondLock );
		}

		BT2_ReleaseController ( base, controller );
	}
}

// --

S32
BT2_ForwardControl ( struct Bluetooth2Base * base, struct BT2MasterRequest * request )
{
	struct MsgPort * port;

	if ( request->Control->Operation < BT2_CONTROL_POLICY_FIRST )
	{
		return ( FALSE );
	}

	base->Exec->ObtainSemaphore ( &base->ControllerLock );

	port = base->PolicyPort;

	if ( port )
	{
		base->Exec->PutMsg ( port, &request->Message );
	}

	base->Exec->ReleaseSemaphore ( &base->ControllerLock );

	return ( port != NULL );
}

// --

static void
DispatchPolicy ( struct Bluetooth2Base * base, struct BT2ControlCall * call )
{
	switch ( call->Operation )
	{
		case BT2_CONTROL_STARTUP_SHARE:
			call->Result = BT2_StartupShareSettings ( base, call->ShareSettings, call->Save );
			break;

		case BT2_CONTROL_AUDIO_OUTPUT:
			call->Result = BT2_AudioOutputSettings ( base, call->AudioSettings, call->Save );
			break;

		case BT2_CONTROL_PEER_ACTION:
			call->Result = BT2_PeerActionForAddress ( base, call->Local, call->Peer, call->PeerAction );
			break;

		case BT2_CONTROL_SHARE_START:
			call->Result = BT2_RunShareForAddress ( base, call->Local, call->Path, FALSE, 0 );
			break;

		case BT2_CONTROL_SHARE_STOP:
			BT2_StopManagedShare ( base );

			call->Result = BT_HCI_OK;
			break;

		case BT2_CONTROL_SHARE_RULE:
			call->Access = BT2_ShareRuleForAddress ( base, call->Local, call->Peer, call->Access );
			call->Result = BT_HCI_OK;
			break;

		default:
			call->Result = BT_HCI_INVALID_ARGUMENT;
			break;
	}
}

// --

static S32
PolicyWorker ( void )
{
	struct ExecIFace *		  exec;
	struct Bluetooth2Base *	  base;
	struct Library *		  library;
	struct DOSIFace *		  dos;
	struct BT2StartupShare	  sharing;
	struct BT2MasterRequest * request;
	struct MsgPort *		  commands;
	struct MsgPort *		  timerPort;
	struct TimeRequest *	  timer;

	U32		delay;
	U32		revision;
	U32		timerMask;
	U32		signals;
	S32		startShare;
	S32		conflict;
	S32		loaded;
	BPTR	environment;
	uint8_t peer[6];

	exec = ExecInterface ();

	base = exec->FindTask ( NULL )->tc_UserData;

	library = exec->OpenLibrary ( "dos.library", 50 );

	if ( ! library )
	{
		return ( RETURN_FAIL );
	}

	dos = (PTR)exec->GetInterface ( library, "main", 1, NULL );

	if ( ! dos )
	{
		exec->CloseLibrary ( library );

		return ( RETURN_FAIL );
	}

	commands = exec->AllocSysObjectTags ( ASOT_PORT, TAG_END );

	timerPort = exec->AllocSysObjectTags ( ASOT_PORT, TAG_END );

	timer = timerPort ? exec->AllocSysObjectTags ( ASOT_IOREQUEST, ASOIOR_Size, sizeof ( *timer ), ASOIOR_ReplyPort, timerPort,
												   TAG_END )
					  : NULL;

	if ( ( ! commands ) || ( ! timer ) || exec->OpenDevice ( TIMERNAME, UNIT_VBLANK, (PTR)timer, 0 ) )
	{
		if ( timer )
		{
			exec->FreeSysObject ( ASOT_IOREQUEST, timer );
		}

		if ( timerPort )
		{
			exec->FreeSysObject ( ASOT_PORT, timerPort );
		}

		if ( commands )
		{
			exec->FreeSysObject ( ASOT_PORT, commands );
		}

		exec->DropInterface ( (PTR)dos );

		exec->CloseLibrary ( library );

		exec->DebugPrintF ( "Bluetooth2: could not initialize the startup policy timer\n" );

		return ( RETURN_FAIL );
	}

	exec->ObtainSemaphore ( &base->ControllerLock );

	base->PolicyPort = commands;

	exec->ReleaseSemaphore ( &base->ControllerLock );

	delay					  = 0;
	timerMask				  = 1UL << timerPort->mp_SigBit;
	timer->Request.io_Command = TR_ADDREQUEST;

	for ( ;; )
	{
		exec->ObtainSemaphore ( &base->BondLock );

		environment = dos->Lock ( "ENV:", SHARED_LOCK );

		loaded = BT2_ReadStartupShare ( dos, &sharing ) >= 0 && environment;

		if ( environment )
		{
			dos->UnLock ( environment );
		}

		conflict = sharing.Enabled && BT2_ReadAutoPeer ( dos, sharing.Local, peer );

		exec->ObtainSemaphore ( &base->ControllerLock );

		if ( ( loaded ) && ( ! base->ShareStartupLoaded ) )
		{
			base->ShareStartupLoaded = TRUE;
			base->ShareStartupArmed	 = sharing.Enabled;
		}

		revision = base->PolicyRevision;

		if ( ! sharing.Enabled )
		{
			base->ShareStartupState = BT2_BOOT_SHARE_DISABLED;
		}
		else if ( conflict )
		{
			base->ShareStartupState = BT2_BOOT_SHARE_CONFLICT;
		}
		else if ( base->ShareSuppressed )
		{
			base->ShareStartupState = BT2_BOOT_SHARE_STOPPED;
		}
		else if ( ! base->ShareStartupArmed )
		{
			base->ShareStartupState = BT2_BOOT_SHARE_NEXT_BOOT;
		}
		else if ( base->ShareState != BT_SHARE_STOPPED )
		{
			base->ShareStartupState = BT2_BOOT_SHARE_RUNNING;
		}
		else if ( ( base->ShareAutomatic ) && ( base->ShareAttemptRevision == revision )
				  && ( base->ShareResult == BT_HCI_BOND_IO_ERROR )
				  && ( base->ShareError.TransportError == ERROR_OBJECT_NOT_FOUND
					   || base->ShareError.TransportError == ERROR_OBJECT_WRONG_TYPE ) )
		{
			base->ShareStartupState = BT2_BOOT_SHARE_FOLDER_ERROR;
		}
		else
		{
			base->ShareStartupState = BT2_BOOT_SHARE_WAITING;
		}

		startShare = ! delay && ( base->ShareStartupState == BT2_BOOT_SHARE_WAITING );

		exec->ReleaseSemaphore ( &base->ControllerLock );

		exec->ReleaseSemaphore ( &base->BondLock );

		if ( startShare )
		{
			BT2_RunShareForAddress ( base, sharing.Local, sharing.Path, TRUE, revision );
		}

		ApplyInputPolicy ( base, dos, &sharing, ! delay );

		delay = ( delay + 1 ) % 5;

		timer->Time.Seconds		 = 1;
		timer->Time.Microseconds = 0;

		exec->SendIO ( (PTR)timer );

		for ( ;; )
		{
			while ( ( request = (PTR)exec->GetMsg ( commands ) ) )
			{
				DispatchPolicy ( base, request->Control );

				exec->ReplyMsg ( &request->Message );
			}

			signals = exec->Wait ( timerMask | ( 1UL << commands->mp_SigBit ) );

			if ( signals & timerMask )
			{
				exec->WaitIO ( (PTR)timer );

				break;
			}
		}
	}
}

// --

void
BT2_StartPolicy ( struct Bluetooth2Base * base )
{
	struct Library *  library;
	struct DOSIFace * dos;
	struct Process *  process;

	process = NULL;

	library = base->Exec->OpenLibrary ( "dos.library", 50 );

	if ( library )
	{
		dos = (PTR)base->Exec->GetInterface ( library, "main", 1, NULL );

		if ( dos )
		{
			process = dos->CreateNewProcTags ( NP_Name, "Bluetooth startup policy", NP_Entry, PolicyWorker, NP_UserData, base,
											   NP_Child, FALSE, NP_StackSize, 32768, NP_WindowPtr, (PTR)-1, TAG_END );

			base->Exec->DropInterface ( (PTR)dos );
		}

		base->Exec->CloseLibrary ( library );
	}

	if ( ! process )
	{
		base->Exec->DebugPrintF ( "Bluetooth2: could not start Bluetooth startup policy\n" );
	}
}

// --
