
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <devices/input.h>
#include <devices/inputevent.h>
#include <proto/exec.h>
#include "bt2_prefs.h"
#include "HIDEvents.h"

struct BTInputDevice
{
	struct BTHIDEvents		Events;
	struct Bluetooth2Base * Base;
	struct BTController *	Controller;
	struct MsgPort *		Port;
	struct IOStdReq *		Request;
	LONG					Error;
	uint8_t					PreviousCode[2];
	uint8_t					PreviousQual[2];
};

// --

static int32_t
SendInputEvent ( void * context, uint8_t kind, uint16_t code, uint16_t qualifier, int16_t x, int16_t y )
{
	struct BTInputDevice * input;
	struct InputEvent	   event;

	LONG status;

	input = context;

	event = (struct InputEvent){ 0 };

	event.ie_Class	   = kind;
	event.ie_Code	   = code;
	event.ie_Qualifier = qualifier;

	if ( kind == IECLASS_RAWKEY )
	{
		event.ie_Prev1DownCode = input->PreviousCode[0];
		event.ie_Prev1DownQual = input->PreviousQual[0];
		event.ie_Prev2DownCode = input->PreviousCode[1];
		event.ie_Prev2DownQual = input->PreviousQual[1];
	}
	else
	{
		event.ie_X = x;
		event.ie_Y = y;
	}

	input->Request->io_Command = IND_ADDEVENT;
	input->Request->io_Data	   = &event;
	input->Request->io_Length  = sizeof ( event );

	status = input->Base->Exec->DoIO ( (APTR)input->Request );

	if ( status )
	{
		input->Error = status;

		return ( FALSE );
	}

	if ( ( kind == IECLASS_RAWKEY ) && ( ! ( code & IECODE_UP_PREFIX ) ) && ( ( code < 0x60 ) || ( code > 0x67 ) ) )
	{
		input->PreviousCode[1] = input->PreviousCode[0];
		input->PreviousCode[0] = code;
		input->PreviousQual[1] = input->PreviousQual[0];
		input->PreviousQual[0] = qualifier;
	}

	return ( TRUE );
}

// --

static int32_t
StartInput ( void * context )
{
	struct BTInputDevice * input;

	input = context;

	// The USB worker owns the reply port and runs the callbacks.
	input->Port = input->Base->Exec->AllocSysObjectTags ( ASOT_PORT, TAG_END );

	if ( ! input->Port )
	{
		return ( FALSE );
	}

	input->Request = input->Base->Exec->AllocSysObjectTags ( ASOT_IOREQUEST, ASOIOR_Size, sizeof ( *input->Request ),
															 ASOIOR_ReplyPort, input->Port, TAG_END );

	if ( input->Request )
	{
		input->Error = input->Base->Exec->OpenDevice ( "input.device", 0, (APTR)input->Request, 0 );

		if ( ! input->Error )
		{
			input->Events.Context = input;
			input->Events.Emit	  = SendInputEvent;
			input->Events.NumLock = TRUE;

			BT2_PrefsInputState ( input->Controller, BT2_PREFS_CONNECTED );

			return ( TRUE );
		}

		input->Base->Exec->FreeSysObject ( ASOT_IOREQUEST, input->Request );
	}

	input->Base->Exec->FreeSysObject ( ASOT_PORT, input->Port );

	return ( FALSE );
}

// --

static int32_t
HandleInput ( void * context, const struct BTHIDReport * report )
{
	struct BTInputDevice * input;

	input = context;

	return ( ApplyHIDReport ( &input->Events, report ) );
}

// --

static void
StopInput ( void * context )
{
	struct BTInputDevice * input;

	input = context;

	ApplyHIDReport ( &input->Events, NULL );

	input->Base->Exec->CloseDevice ( (APTR)input->Request );

	input->Base->Exec->FreeSysObject ( ASOT_IOREQUEST, input->Request );

	input->Base->Exec->FreeSysObject ( ASOT_PORT, input->Port );
}

// --
enum BTHCIResult
BT2_PrefsRunInput ( struct BTController * controller, const uint8_t * peer )
{
	static const struct BTInputDevice	  emptyInput = { 0 };
	static const struct BTHIDInput		  emptyHID	 = { 0 };
	static const struct BTControllerProbe emptyProbe = { 0 };
	struct BTInputDevice				  input;
	struct BTHIDInput					  hid;
	struct BTControllerLocation			  location;
	struct BTControllerProbe			  probe;
	struct BTHCIError					  error;
	enum BTHCIResult					  result;

	S32 log;

	controller->Base->Exec->CopyMem ( &emptyInput, &input, sizeof ( input ) );

	controller->Base->Exec->CopyMem ( &emptyHID, &hid, sizeof ( hid ) );

	controller->Base->Exec->CopyMem ( &emptyProbe, &probe, sizeof ( probe ) );

	input.Base		 = controller->Base;
	input.Controller = controller;
	hid.Context		 = &input;
	hid.Start		 = StartInput;
	hid.Report		 = HandleInput;
	hid.Stop		 = StopInput;
	probe.HID		 = &hid.Monitor;
	probe.Error		 = &error;
	error			 = (struct BTHCIError){ 0 };

	input.Base->Exec->CopyMem ( peer, hid.Monitor.Address, sizeof ( hid.Monitor.Address ) );

	result = BT2_RequestForController ( input.Base, controller->Unit, controller, &location, &probe );

	input.Base->Exec->ObtainSemaphore ( &input.Base->ControllerLock );

	log = ( controller->Port ) && ( result != controller->LastResult );

	input.Base->Exec->ReleaseSemaphore ( &input.Base->ControllerLock );

	if ( ( log ) && ( result != BT_HCI_OK ) && ( result != BT_RFCOMM_CANCELLED ) && ( result != BT_HID_DISCONNECTED )
		 && ( result != BT_HCI_CONTROLLER_BUSY ) )
	{
		input.Base->Exec->DebugPrintF (
			"Bluetooth HID: unit %lu %s failed, result %lu, transport %ld, HCI $%04lx status $%02lx, input %ld\n",
			controller->Unit, input.Events.Context ? "input" : "connect", (U32)result, error.TransportError, (U32)error.Opcode,
			(U32)error.Status, input.Error );
	}

	return ( result );
}

// --
