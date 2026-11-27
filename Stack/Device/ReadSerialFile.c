
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/dos.h>
#include "bt2_device.h"
#include "bt2_rfcomm.h"

// --

int32_t
ReadSerialFile ( void * context, uint8_t * data, uint32_t capacity )
{
	struct BTFileSource * source;

	S32 length;

	source = context;

	length = source->DOS->Read ( source->Handle, data, capacity );

	if ( length < 0 )
	{
		*source->FileError = source->DOS->IoErr ();
	}

	return ( length );
}

// --
