
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <dos/var.h>
#include "bt2_share.h"

// --

void
BT2_ShareRuleName ( STRPTR name, const uint8_t * local, const uint8_t * peer )
{
	static const char suffix[] = "-share";

	U32 index;

	BT2_BondName ( name, local, peer );

	for ( index = 0; index < sizeof ( suffix ); index++ )
	{
		name[BT_BOND_NAME_SIZE - 1 + index] = suffix[index];
	}
}

// --

uint8_t
BT2_ShareAccess ( struct BTShareSession * session, uint8_t * key )
{
	struct BTShareServer * server;

	char	name[BT_SHARE_RULE_NAME_SIZE];
	char	rule[2];
	uint8_t saved[17];
	U32		index;
	S32		length;
	S32		match;

	server = session->Server;
	match  = TRUE;

	BT2_BondName ( name, session->Local, session->Peer );

	server->Base->Exec->ObtainSemaphore ( &server->Base->BondLock );

	length = server->DOS->GetVar ( name, (STRPTR)saved, sizeof ( saved ), BT_BOND_FLAGS | GVF_DONT_NULL_TERM );

	for ( index = 0; index < 16; index++ )
	{
		if ( length == 16 )
		{
			if ( key )
			{
				key[index] = saved[index];
			}
			else if ( session->Key[index] != saved[index] )
			{
				match = FALSE;
			}
		}

		( (volatile uint8_t *)saved )[index] = 0;
	}

	( (volatile uint8_t *)saved )[16] = 0;

	if ( ( length == 16 ) && ( match ) )
	{
		BT2_ShareRuleName ( name, session->Local, session->Peer );

		length = server->DOS->GetVar ( name, rule, sizeof ( rule ), BT_BOND_FLAGS | GVF_DONT_NULL_TERM );
	}
	else
	{
		length = -1;
	}

	server->Base->Exec->ReleaseSemaphore ( &server->Base->BondLock );

	if ( ( length != 1 ) || ( rule[0] < '1' ) || ( rule[0] > '3' ) )
	{
		return ( 0 );
	}

	return ( rule[0] - '0' );
}

// --
