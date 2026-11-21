
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/dos.h>
#include "bt2_all.h"
#include "bt2_rfcomm.h"

// --

struct BTFileSource
{
	struct DOSIFace *DOS;
	int32_t *FileError;
	BPTR Handle;
};

// --

static int32_t
ReadSerialFile( void *context, uint8_t *data, uint32_t capacity )
{
	struct BTFileSource *source;

	S32 length;

	source = context;

	length = source->DOS->Read( source->Handle, data, capacity );

	if ( length < 0 )
	{
		*source->FileError = source->DOS->IoErr();
	}

	return ( length );
}

// --
