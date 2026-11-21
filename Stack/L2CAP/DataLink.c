
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <stddef.h>
#include "bt2_l2cap.h"

// --

#define HCI_READ_BUFFER_SIZE 0x1005

// --

enum BTHCIResult
BT2_OpenDataLinkWithKey( struct BTL2CAPLink *link, const struct BTACLTransport *transport,
	const uint8_t *address, const uint8_t *key, struct BTHCIError *error )
{
	struct BTHCIError cleanupError;
	struct BTPairKeys keys;
	enum BTHCIResult result;

	uint8_t bufferSize[8];
	uint32_t byte;

	if ( ( ! address ) || ( ! transport ) || ( ! transport->ReadACLChunk ) || ( ! transport->WriteACL ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	result = BT_HCICommand( & transport->HCI, HCI_READ_BUFFER_SIZE, bufferSize, sizeof( bufferSize ), error );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	link->Transport = transport;

	link->PacketSize = BT_ReadLE16( & bufferSize[1] );

	link->CreditLimit = BT_ReadLE16( & bufferSize[4] );

	if ( ( link->PacketSize < BT_L2CAP_HEADER + BT_L2CAP_SIGNAL_HEADER ) || ( ! link->CreditLimit ) )
	{
		return ( BT_HCI_UNSUPPORTED );
	}

	link->Credits = link->CreditLimit;
	link->PeerMTU = BT_L2CAP_MTU;
	link->Cancel = NULL;
	link->ExtraReadTimeouts = 0;
	link->Handle = BT_CLASSIC_INVALID_HANDLE;
	link->Peer.PINLength = 0;

	for( byte = 0; byte < sizeof( link->Peer.Address ); byte++ )
	{
		link->Peer.Address[byte] = address[byte];
	}

	keys.Saved = key;
	keys.NewValid = 0;
	keys.Sent = 0;

	result = BT2_OpenClassicLink( & transport->HCI, & link->Peer, key ? & keys : NULL, & link->Handle, error );

	if ( ( result == BT_HCI_OK ) && ( key ) )
	{
		result = BT2_AuthenticateClassicLink( & transport->HCI, & link->Peer, & keys, link->Handle, error );
	}

	if ( result != BT_HCI_OK )
	{
		BT2_CloseClassicLink( & transport->HCI, & link->Peer, NULL, link->Handle, & cleanupError );
	}

	return ( result );
}

// --

enum BTHCIResult
BT2_OpenDataLink( struct BTL2CAPLink *link, const struct BTACLTransport *transport,
	const uint8_t *address, struct BTHCIError *error )
{
	return ( BT2_OpenDataLinkWithKey( link, transport, address, NULL, error ) );
}

// --

enum BTHCIResult
BT2_CloseDataLink( struct BTL2CAPLink *link, enum BTHCIResult previous, struct BTHCIError *error )
{
	struct BTHCIError cleanupError;
	enum BTHCIResult result;

	result = BT2_CloseClassicLink( & link->Transport->HCI, & link->Peer, NULL, link->Handle, & cleanupError );

	if ( ( previous == BT_HCI_OK ) && ( result != BT_HCI_OK ) )
	{
		*error = cleanupError;

		return ( result );
	}

	return ( previous );
}

// --
