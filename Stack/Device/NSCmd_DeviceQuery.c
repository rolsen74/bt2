
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_all.h"

// --

static void
NSCmd_DeviceQuery( struct IORequest *ioreq )
{
	static const U16 commands[] = { NSCMD_DEVICEQUERY, 0 };

	struct IOStdReq *request;
	struct NSDeviceQueryResult *result;

	if ( ioreq->io_Message.mn_Length < sizeof( *request ) )
	{
		ioreq->io_Error = IOERR_BADLENGTH;

		return;
	}

	request = (PTR) ioreq;
	result = request->io_Data;
	request->io_Actual = 0;

	if ( ( ! result ) || ( request->io_Length < sizeof( *result ) ) )
	{
		ioreq->io_Error = IOERR_BADLENGTH;

		return;
	}

	result->DevQueryFormat = 0;
	result->SizeAvailable = sizeof( *result );
	result->DeviceType = NSDEVTYPE_UNKNOWN;
	result->DeviceSubType = 0;
	result->SupportedCommands = (U16 *) commands;
	request->io_Actual = sizeof( *result );
	ioreq->io_Error = 0;
}

// --
