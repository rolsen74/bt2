
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <interfaces/usb2units.h>
#include "bt2_device.h"

// --

void
_manager_BeginIO ( struct DeviceManagerInterface * self, struct IORequest * ioreq )
{
	struct Bluetooth2Base * base;

	base = (PTR)self->Data.LibBase;

	if ( ioreq->io_Command == NSCMD_DEVICEQUERY )
	{
		NSCmd_DeviceQuery ( ioreq );
	}
	else if ( ioreq->io_Command == USB2_CMD_ENUM_UNITS )
	{
		EnumUnits ( base, ioreq );
	}
	else
	{
		ioreq->io_Error = IOERR_NOCMD;
	}

	if ( ! ( ioreq->io_Flags & IOF_QUICK ) )
	{
		ioreq->io_Message.mn_Node.ln_Type = NT_MESSAGE;

		base->Exec->ReplyMsg ( &ioreq->io_Message );
	}
}

// --
