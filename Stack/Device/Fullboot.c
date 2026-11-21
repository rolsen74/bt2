
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
Full_Boot ( PTR dummy __attribute__ ( ( unused ) ), PTR seglist __attribute__ ( ( unused ) ), struct ExecBase * sysbase )
{
	struct ExecIFace *		exec;
	struct Bluetooth2Base * base;

	exec = (PTR)sysbase->MainInterface;

	base = (PTR)exec->FindName ( &sysbase->DeviceList, BLUETOOTH2_NAME );

	if ( base )
	{
		if ( BT2_MasterStart ( base ) )
		{
			exec->DebugPrintF ( "Bluetooth : Full Boot\n" );
		}
		else
		{
			exec->DebugPrintF ( "Bluetooth : could not start the master process\n" );
		}
	}

	return ( NULL );
}

// --

struct Resident Bluetooth2FullbootResident __attribute__ ( ( used ) ) = //
	{ .rt_MatchWord = RTC_MATCHWORD,
	  .rt_MatchTag	= (PTR)&Bluetooth2FullbootResident,
	  .rt_EndSkip	= (PTR)( &Bluetooth2FullbootResident + 1 ),
	  .rt_Flags		= RTF_NATIVE | RTF_AFTERDOS,
	  .rt_Version	= BLUETOOTH2_VERSION,
	  .rt_Type		= NT_UNKNOWN,
	  .rt_Pri		= -48,
	  .rt_Name		= "Bluetooth2 - Fullboot",
	  .rt_IdString	= "Bluetooth2 - Fullboot",
	  .rt_Init		= (PTR)Full_Boot };

// --
