
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/dos.h>
#include <dos/var.h>
#include "bt2_all.h"
#include "bt2_hci.h"

// --

#define BT_BOND_PREFIX "Bluetooth2/"
#define BT_BOND_NAME_SIZE ( sizeof( BT_BOND_PREFIX ) + 25 )
#define BT_BOND_FLAGS ( GVF_GLOBAL_ONLY | GVF_BINARY_VAR )

// --

static STRPTR
AppendAddress( STRPTR text, const uint8_t *address )
{
	static const char hex[] = "0123456789abcdef";

	U32 byte;

	for( byte = 0; byte < 6; byte++ )
	{
		*text++ = hex[address[byte] >> 4];
		*text++ = hex[address[byte] & 0x0f];
	}

	return ( text );
}

// --

static void
BondName( STRPTR name, const uint8_t *local, const uint8_t *peer )
{
	U32 byte;

	for( byte = 0; byte < sizeof( BT_BOND_PREFIX ) - 1; byte++ )
	{
		*name++ = BT_BOND_PREFIX[byte];
	}

	name = AppendAddress( name, local );

	*name++ = '-';

	name = AppendAddress( name, peer );

	*name = 0;
}

// --

static enum BTHCIResult
RunBond( struct Bluetooth2Base *base, struct DOSIFace *dos, const struct BTHCITransport *transport,
	struct BTBondRequest *bond, const char *name, struct BTHCIError *error )
{
	struct BTPairKeys keys;
	enum BTHCIResult result;

	uint8_t saved[BT_LINK_KEY_SIZE + 1];
	U32 byte;
	S32 length;
	S32 savedOK;

	keys.Saved = NULL;
	keys.NewValid = 0;
	keys.Sent = 0;

	if ( bond->Reconnect )
	{
		base->Exec->ObtainSemaphore( & base->BondLock );

		length = dos->GetVar( name, (STRPTR) saved, sizeof( saved ), BT_BOND_FLAGS | GVF_DONT_NULL_TERM );

		savedOK = ( length < 0 ) && ( dos->IoErr() == ERROR_OBJECT_NOT_FOUND );

		base->Exec->ReleaseSemaphore( & base->BondLock );

		if ( length != BT_LINK_KEY_SIZE )
		{
			result = savedOK ? BT_HCI_NO_SAVED_KEY : BT_HCI_BOND_IO_ERROR;

			goto clear_keys;
		}

		keys.Saved = saved;
	}

	result = BT2_PairWithKeys( transport, & bond->Peer, & keys, error );

	bond->SentSavedKey = keys.Sent;

	if ( ( result == BT_HCI_OK ) && ( ! bond->Reconnect ) )
	{
		if ( keys.NewValid )
		{
			base->Exec->ObtainSemaphore( & base->BondLock );

			savedOK = dos->SetVar( name, (STRPTR) keys.New, sizeof( keys.New ), BT_BOND_FLAGS | GVF_SAVE_VAR );

			base->Exec->ReleaseSemaphore( & base->BondLock );

			if ( ! savedOK )
			{
				result = BT_HCI_BOND_IO_ERROR;
			}
		}
		else
		{
			result = BT_HCI_NO_NEW_KEY;
		}
	}

clear_keys:
	for( byte = 0; byte < sizeof( saved ); byte++ )
	{
		((volatile uint8_t *)saved)[byte] = 0;
	}

	for( byte = 0; byte < sizeof( keys.New ); byte++ )
	{
		((volatile uint8_t *)keys.New)[byte] = 0;
	}

	return ( result );
}

// --

enum BTHCIResult
BT2_Bond( struct Bluetooth2Base *base, const struct BTHCITransport *transport,
	struct BTBondRequest *bond, struct BTHCIError *error )
{
	struct BTControllerInfo info;
	struct DOSIFace *dos;
	struct Library *dosBase;
	enum BTHCIResult result;

	char name[BT_BOND_NAME_SIZE];

	if ( ( ! bond ) || ( bond->Peer.PINLength > BT_PIN_MAX ) ||
		( bond->Reconnect ? bond->Peer.PINLength != 0 : bond->Peer.PINLength == 0 ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	bond->SentSavedKey = 0;

	result = BT_ReadControllerInfo( transport, & info, error );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	dosBase = base->Exec->OpenLibrary( "dos.library", 50 );

	if ( ! dosBase )
	{
		return ( BT_HCI_BOND_IO_ERROR );
	}

	dos = (PTR) base->Exec->GetInterface( dosBase, "main", 1, NULL );

	if ( dos )
	{
		BondName( name, info.Address, bond->Peer.Address );

		result = RunBond( base, dos, transport, bond, name, error );

		base->Exec->DropInterface( (PTR) dos );
	}
	else
	{
		result = BT_HCI_BOND_IO_ERROR;
	}

	base->Exec->CloseLibrary( dosBase );

	return ( result );
}

// --
