
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_device.h"
#include <interfaces/usb2units.h>

// --

void
EnumUnits ( struct Bluetooth2Base * base, struct IORequest * ioreq )
{
	struct BTController * controller;
	struct IOStdReq *	  request;
	struct USB2UnitInfo	  entry;

	uint8_t * buffer;
	U32		  count;

	if ( ioreq->io_Message.mn_Length < sizeof ( *request ) )
	{
		ioreq->io_Error = IOERR_BADLENGTH;

		return;
	}

	request			   = (PTR)ioreq;
	request->io_Actual = 0;

	if ( request->io_Offset != USB2_UNIT_LIST_VERSION )
	{
		request->io_Error = IOERR_NOCMD;

		return;
	}

	base->Exec->ObtainSemaphore ( &base->ControllerLock );

	count = 0;

	for ( controller = base->Controllers; controller; controller = controller->Next )
	{
		count++;
	}

	request->io_Actual = count * sizeof ( entry );
	request->io_Error  = 0;

	if ( ( request->io_Actual > request->io_Length ) || ( ( count ) && ( ! request->io_Data ) ) )
	{
		request->io_Error = IOERR_BADLENGTH;
	}
	else
	{
		buffer = request->io_Data;

		for ( controller = base->Controllers; controller; controller = controller->Next )
		{
			entry.Number = controller->Unit;
			entry.Flags	 = controller->Exclusive ? USB2_UNIT_BUSY : 0;

			base->Exec->CopyMem ( &entry, buffer, sizeof ( entry ) );

			buffer += sizeof ( entry );
		}
	}

	base->Exec->ReleaseSemaphore ( &base->ControllerLock );
}

// --
