
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_control.h"

// --

#define BT2_ACTIVE_JOBS 16

// --

static struct BT2ManagedJob *
FindJob ( struct BT2ManagedJob * job, U32 id )
{
	while ( job && ( job->ID != id ) )
	{
		job = job->Next;
	}

	return ( job );
}

// --

void
BT2_ClearJobPIN ( struct BT2ManagedJob * job )
{
	volatile uint8_t * pin;

	U32 index;

	pin = job->Bond.Peer.PIN;

	for ( index = 0; index < BT_PIN_MAX; index++ )
	{
		pin[index] = 0;
	}

	job->Bond.Peer.PINLength = 0;
}

// --

static void
Submit ( struct Bluetooth2Base * base, struct BT2ControlCall * call )
{
	struct BTController *		controller;
	struct BTController *		selected;
	struct BTControllerLocation location;
	struct BT2MasterRequest		request;
	struct BT2ManagedJob *		job;

	U32 unit;
	S32 ready;

	call->Result = BT_HCI_CONTROLLER_BUSY;

	if ( ( base->ActiveJobs >= BT2_ACTIVE_JOBS ) || ( call->JobType == BT2_MANAGED_WAV && base->PlaybackJob ) )
	{
		return;
	}

	base->Exec->ObtainSemaphore ( &base->ControllerLock );

	selected = call->JobType == BT2_MANAGED_WAV ? BT2_PrefsFindAdapter ( base, call->Audio->Local ) : NULL;

	for ( controller = base->Controllers; controller; controller = controller->Next )
	{
		if ( ( call->JobType == BT2_MANAGED_WAV ) ? ( controller == selected )
												  : ( controller->Registration == call->Registration ) )
		{
			controller->References++;
			unit  = controller->Unit;
			ready = controller->Ready;

			break;
		}
	}

	base->Exec->ReleaseSemaphore ( &base->ControllerLock );

	if ( ! controller )
	{
		call->Result = BT_HCI_NO_CONTROLLER;

		return;
	}

	if ( ( call->JobType == BT2_MANAGED_PAIR ) && ( ! ready ) )
	{
		call->Result = BT_HCI_NO_CONTROLLER;

		goto release;
	}

	job = base->Exec->AllocVecTags ( sizeof ( *job ), AVT_ClearWithValue, 0, TAG_END );

	if ( ! job )
	{
		call->Result = BT_HCI_NO_MEMORY;

		goto release;
	}

	job->Probe.Error				= &job->Error;
	job->Probe.Message.mn_ReplyPort = base->CompletionPort;

	if ( call->JobType == BT2_MANAGED_PAIR )
	{
		base->Exec->CopyMem ( call->Pair, &job->Bond.Peer, sizeof ( job->Bond.Peer ) );

		job->Probe.Bond = &job->Bond;
	}
	else if ( call->JobType == BT2_MANAGED_WAV )
	{
		base->Exec->CopyMem ( call->Path, job->Path, call->Length );

		base->Exec->CopyMem ( call->Audio->Peer, job->Audio.Address, sizeof ( job->Audio.Address ) );

		job->Audio.Path	 = job->Path;
		job->Probe.Audio = &job->Audio;
	}
	else
	{
		job->Inquiry.Devices  = job->Devices;
		job->Inquiry.Capacity = BT2_SCAN_CAPACITY;
		job->Probe.Inquiry	  = &job->Inquiry;
	}

	request.Expected = controller;
	request.Unit	 = unit;
	request.Probe	 = &job->Probe;
	request.Route	 = &location;
	request.Caller	 = NULL;

	call->Result = BT2_QueueProbe ( base, &request );

	if ( call->Result != BT_HCI_OK )
	{
		BT2_ClearJobPIN ( job );

		base->Exec->FreeVec ( job );

		goto release;
	}

	do
	{
		job->ID = ++base->NextJob;
	} while ( ! job->ID || FindJob ( base->Jobs, job->ID ) || FindJob ( base->Results, job->ID ) );

	job->Controller = request.Controller;
	job->Next		= base->Jobs;
	base->Jobs		= job;
	base->ActiveJobs++;
	call->ID = job->ID;

	if ( call->JobType == BT2_MANAGED_WAV )
	{
		base->PlaybackJob = job->ID;
	}

release:
	BT2_ReleaseController ( base, controller );
}

// --

static void
Result ( struct Bluetooth2Base * base, struct BT2ControlCall * call )
{
	struct BT2OperationStatus * status;
	struct BT2ManagedJob *		job;

	U32 count;

	status = call->Status;

	job = FindJob ( base->Jobs, call->ID );

	status->Pending = job != NULL;

	if ( ! job )
	{
		job = FindJob ( base->Results, call->ID );
	}

	if ( ! job )
	{
		call->Result = BT_HCI_INVALID_ARGUMENT;

		return;
	}

	call->Result = BT_HCI_OK;

	if ( status->Pending )
	{
		return;
	}

	status->Result = job->Probe.Result;
	status->Error  = job->Error;
	count		   = job->Inquiry.Count;

	if ( count > BT2_SCAN_CAPACITY )
	{
		count = BT2_SCAN_CAPACITY;
	}

	status->Count	  = count;
	status->Truncated = job->Inquiry.Truncated || ( count > call->Capacity );

	if ( count > call->Capacity )
	{
		count = call->Capacity;
	}

	if ( ( count ) && ( call->Devices ) )
	{
		base->Exec->CopyMem ( job->Devices, call->Devices, count * sizeof ( *call->Devices ) );
	}
}

// --

void
BT2_ControlDispatch ( struct Bluetooth2Base * base, struct BT2ControlCall * call )
{
	struct BTController *  controller;
	struct BT2ManagedJob * job;

	switch ( call->Operation )
	{
		case BT2_CONTROL_ADAPTERS:
			call->Count = 0;

			base->Exec->ObtainSemaphore ( &base->ControllerLock );

			for ( controller = base->Controllers; controller; controller = controller->Next )
			{
				if ( call->Count < call->Capacity )
				{
					call->Adapters[call->Count].Registration = controller->Registration;

					BT2_FillAdapter ( base, controller, &call->Adapters[call->Count].Adapter );
				}

				call->Count++;
			}

			base->Exec->ReleaseSemaphore ( &base->ControllerLock );

			break;

		case BT2_CONTROL_SUBMIT:
			Submit ( base, call );
			break;

		case BT2_CONTROL_OPERATION:
			Result ( base, call );
			break;

		case BT2_CONTROL_PLAYBACK:
			call->ID = base->PlaybackJob;
			break;

		case BT2_CONTROL_CANCEL:
			job = FindJob ( base->Jobs, call->ID );

			if ( ! job )
			{
				call->Result = BT_HCI_INVALID_ARGUMENT;
			}
			else
			{
				if ( job->Probe.Audio )
				{
					__atomic_store_n ( &job->Audio.Cancel, 1, __ATOMIC_RELAXED );
				}

				if ( BT2_CancelQueued ( base, job->Controller, &job->Probe ) )
				{
					job->Probe.Result = BT_RFCOMM_CANCELLED;

					BT2_ControlComplete ( base, &job->Probe.Message );

					call->Result = BT_HCI_OK;
				}
				else
				{
					// Scan and bond probes can only be cancelled before claim.
					call->Result = job->Probe.Audio ? BT_HCI_OK : BT_HCI_CONTROLLER_BUSY;
				}
			}
			break;

		default:
			call->Result = BT_HCI_NO_CONTROLLER;
			break;
	}
}

// --
