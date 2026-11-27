
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_control.h"

// --

#define BT2_RETAINED_JOBS 16

// --

void
BT2_ControlComplete ( struct Bluetooth2Base * base, struct Message * message )
{
	struct BT2ManagedJob ** link;
	struct BT2ManagedJob *	job;

	U32 count;

	for ( link = &base->Jobs; *link; link = &( *link )->Next )
	{
		if ( &( *link )->Probe.Message == message )
		{
			break;
		}
	}

	job = *link;

	if ( ! job )
	{
		return;
	}

	*link = job->Next;

	if ( base->PlaybackJob == job->ID )
	{
		base->PlaybackJob = 0;
	}

	base->Exec->ObtainSemaphore ( &base->ControllerLock );

	if ( ! job->Controller->Port )
	{
		job->Probe.Result = BT_HCI_NO_CONTROLLER;
	}

	base->Exec->ReleaseSemaphore ( &base->ControllerLock );

	BT2_RetireProbe ( base, job->Controller, &job->Probe );

	BT2_ClearJobPIN ( job );

	job->Controller = NULL;
	job->Next		= base->Results;
	base->Results	= job;
	base->ActiveJobs--;

	link = &base->Results;

	for ( count = 0; ( *link ) && ( count < BT2_RETAINED_JOBS ); count++ )
	{
		link = &( *link )->Next;
	}

	if ( *link )
	{
		base->Exec->FreeVec ( *link );

		*link = NULL;
	}
}

// --
