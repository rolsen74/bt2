
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_share.h"

// --

static S32
ShareFolderError ( struct DOSIFace * dos, const char * path )
{
	char volume[BT_SHARE_PATH_SIZE];
	U32	 index;
	S32	 error;
	BPTR lock;

	error = dos->IoErr ();

	if ( error != ERROR_OBJECT_NOT_FOUND )
	{
		return ( error );
	}

	for ( index = 0; ( index < sizeof ( volume ) - 1 ) && ( path[index] ); index++ )
	{
		volume[index] = path[index];

		if ( path[index] == ':' )
		{
			volume[index + 1] = 0;

			lock = dos->Lock ( volume, SHARED_LOCK );

			if ( lock )
			{
				dos->UnLock ( lock );
			}
			else
			{
				error = dos->IoErr ();

				if ( error == ERROR_OBJECT_NOT_FOUND )
				{
					error = ERROR_DEVICE_NOT_MOUNTED;
				}
			}

			break;
		}
	}

	return ( error );
}

// --

enum BTHCIResult
_main_fileshare_Serve ( struct Bluetooth2IFace * self, const struct BTACLTransport * transport, struct BTFileShare * share,
						struct BTHCIError * error )
{
	struct BTShareServer server;
	struct Library *	 library;
	struct ExamineData * directory;
	enum BTHCIResult	 result;

	if ( ( ! share ) || ( ! share->Path[0] ) || ( share->Path[BT_SHARE_PATH_SIZE - 1] ) || ( ! error ) || ( ! transport )
		 || ( ! transport->WriteACL ) || ( ! transport->ReadACLChunk ) || ( ! transport->HCI.StartCommand )
		 || ( ! transport->HCI.ReadEventChunk ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	error->Opcode		  = 0;
	error->TransportError = 0;
	error->Status		  = 0;
	server.Base			  = (PTR)self->Data.LibBase;
	server.Request		  = share;
	result				  = BT_HCI_BOND_IO_ERROR;

	library = server.Base->Exec->OpenLibrary ( "dos.library", 53 );

	if ( ! library )
	{
		return ( result );
	}

	server.DOS = (PTR)server.Base->Exec->GetInterface ( library, "main", 1, NULL );

	if ( server.DOS )
	{
		server.Root = server.DOS->Lock ( share->Path, SHARED_LOCK );

		if ( server.Root )
		{
			directory = server.DOS->ExamineObjectTags ( EX_LockInput, server.Root, EX_DataFields, EXF_TYPE, TAG_END );

			if ( directory )
			{
				if ( EXD_IS_DIRECTORY ( directory ) )
				{
					result = BT2_RunShare ( &server, transport, error );
				}
				else
				{
					error->TransportError = ERROR_OBJECT_WRONG_TYPE;
				}

				server.DOS->FreeDosObject ( DOS_EXAMINEDATA, directory );
			}
			else
			{
				error->TransportError = server.DOS->IoErr ();
			}

			server.DOS->UnLock ( server.Root );
		}
		else
		{
			error->TransportError = ShareFolderError ( server.DOS, share->Path );
		}

		server.Base->Exec->DropInterface ( (PTR)server.DOS );
	}

	server.Base->Exec->CloseLibrary ( library );

	return ( result );
}

// --
