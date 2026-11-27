
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

enum BTHCIResult
_main_serialcapture_Receive ( struct Bluetooth2IFace * self, const struct BTACLTransport * transport,
							  struct BTRFCOMMCapture * capture, struct BTHCIError * error )
{
	struct Bluetooth2Base * base;
	struct BTFileSink		sink;
	struct Library *		dosBase;
	enum BTHCIResult		result;

	if ( ( ! capture ) || ( ! capture->Path ) || ( ! capture->Expected ) || ( ! capture->Peer.Channel )
		 || ( capture->Peer.Channel > 30 ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	if ( __atomic_load_n ( &capture->Cancel, __ATOMIC_RELAXED ) )
	{
		return ( BT_RFCOMM_CANCELLED );
	}

	base		 = (PTR)self->Data.LibBase;
	sink.Request = capture;
	result		 = BT_RFCOMM_FILE_ERROR;

	dosBase = base->Exec->OpenLibrary ( "dos.library", 50 );

	if ( ! dosBase )
	{
		return ( result );
	}

	sink.DOS = (PTR)base->Exec->GetInterface ( dosBase, "main", 1, NULL );

	if ( sink.DOS )
	{
		result = ReceiveFile ( base, transport, &sink, error );

		base->Exec->DropInterface ( (PTR)sink.DOS );
	}

	base->Exec->CloseLibrary ( dosBase );

	return ( result );
}

// --
