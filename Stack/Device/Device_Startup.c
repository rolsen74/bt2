
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_device.h"

// --

PTR
ROMInit ( PTR dummy __attribute__ ( ( unused ) ), PTR seglist, struct ExecBase * sysbase )
{
	struct ExecIFace *		exec;
	struct Bluetooth2Base * base;

	exec = (PTR)sysbase->MainInterface;

	if ( exec->FindName ( &sysbase->DeviceList, BLUETOOTH2_NAME ) )
	{
		return ( NULL );
	}

	base = (PTR)exec->CreateLibraryTags ( //
		CLT_DataSize, sizeof ( *base ),	  //
		CLT_Interfaces, DeviceInterfaces, //
		CLT_NoLegacyIFace, TRUE,		  //
		TAG_END );

	if ( ! base )
	{
		return ( NULL );
	}

	base->Exec				   = exec;
	base->SegList			   = seglist;
	base->Controllers		   = NULL;
	base->MasterPort		   = NULL;
	base->CompletionPort	   = NULL;
	base->PolicyPort		   = NULL;
	base->NextRegistration	   = 0;
	base->Jobs				   = NULL;
	base->Results			   = NULL;
	base->NextJob			   = 0;
	base->ActiveJobs		   = 0;
	base->PolicyRevision	   = 0;
	base->ShareSuppressed	   = FALSE;
	base->ShareStartupArmed	   = FALSE;
	base->ShareStartupLoaded   = FALSE;
	base->ShareAutomatic	   = FALSE;
	base->ShareAttemptRevision = 0;
	base->ShareStartupState	   = 0;
	base->PlaybackJob		   = 0;
	base->StoppedInputs		   = NULL;
	base->ShareController	   = NULL;
	base->ShareTask			   = NULL;
	base->ShareState		   = BT_SHARE_STOPPED;
	base->ShareResult		   = BT_HCI_OK;
	base->ShareError		   = (struct BTHCIError){ 0 };
	base->Share.Path[0]		   = 0;
	base->Unit.unit_OpenCnt	   = 0;

	base->Device.dd_Library.lib_Node.ln_Type = NT_DEVICE;
	base->Device.dd_Library.lib_Node.ln_Name = BLUETOOTH2_NAME;
	base->Device.dd_Library.lib_Flags		 = LIBF_SUMUSED | LIBF_CHANGED;
	base->Device.dd_Library.lib_Version		 = BLUETOOTH2_VERSION;
	base->Device.dd_Library.lib_Revision	 = BLUETOOTH2_REVISION;
	base->Device.dd_Library.lib_IdString	 = BLUETOOTH2_ID;

	exec->InitSemaphore ( &base->ControllerLock );

	exec->InitSemaphore ( &base->BondLock );

	exec->AddDevice ( &base->Device );

	return ( NULL );
}

// --
