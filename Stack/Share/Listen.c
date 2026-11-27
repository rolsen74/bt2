
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_share.h"

// --

static enum BTHCIResult
PageScan ( const struct BTHCITransport * transport, uint8_t enable, struct BTHCIError * error )
{
	uint8_t command[] = { 0x1a, 0x0c, 1, 0 };
	uint8_t reply[1];

	command[3] = enable;

	return ( BT2_CompleteCommand ( transport, command, sizeof ( command ), reply, sizeof ( reply ), error ) );
}

// --

static enum BTHCIResult
ServePeer ( struct BTShareServer * server, struct BTL2CAPLink * link, const uint8_t * address, struct BTHCIError * error )
{
	static const struct BTShareSession empty = { 0 };
	struct BTHCIError				   cleanup;
	struct BTShareSession			   session;
	struct BTPairKeys				   keys;
	enum BTHCIResult				   result;

	uint8_t reject[] = { 0x0a, 0x04, 7, 0, 0, 0, 0, 0, 0, 0x0e };
	U32		byte;

	server->Base->Exec->CopyMem ( &empty, &session, sizeof ( session ) );

	session.Server		 = server;
	link->Handle		 = BT_CLASSIC_INVALID_HANDLE;
	link->Credits		 = link->CreditLimit;
	link->Peer.PINLength = 0;

	for ( byte = 0; byte < 6; byte++ )
	{
		session.Local[byte]		 = server->Request->Local[byte];
		session.Peer[byte]		 = address[5 - byte];
		link->Peer.Address[byte] = session.Peer[byte];
		reject[3 + byte]		 = address[byte];
	}

	session.Access = BT2_ShareAccess ( &session, session.Key );

	if ( ! session.Access )
	{
		link->Transport->HCI.StartCommand ( link->Transport->HCI.Context, reject, sizeof ( reject ) );

		return ( BT_HCI_OK );
	}

	keys.Saved	  = session.Key;
	keys.NewValid = FALSE;
	keys.Sent	  = FALSE;

	result = BT2_AcceptClassicLink ( &link->Transport->HCI, &link->Peer, &keys, &link->Handle, error );

	if ( result == BT_HCI_OK )
	{
		result = BT2_AuthenticateClassicLink ( &link->Transport->HCI, &link->Peer, &keys, link->Handle, error );
	}

	if ( ( result == BT_HCI_OK ) && ( keys.Sent ) && ( BT2_ShareAccess ( &session, NULL ) ) )
	{
		result = PageScan ( &link->Transport->HCI, 0, error );

		if ( result == BT_HCI_OK )
		{
			BT2_ShareTransport ( &session, link, error );
		}
	}

	BT2_ShareClose ( &session );

	for ( byte = 0; byte < sizeof ( session.Key ); byte++ )
	{
		( (volatile uint8_t *)session.Key )[byte] = 0;
	}

	// Closing an incomplete link resets the controller before listening again.
	if ( ! BT2_LinkCancelled ( link ) )
	{
		BT2_CloseClassicLink ( &link->Transport->HCI, &link->Peer, NULL, link->Handle, &cleanup );

		return ( PageScan ( &link->Transport->HCI, 2, error ) );
	}

	return ( BT_RFCOMM_CANCELLED );
}

// --

enum BTHCIResult
BT2_RunShare ( struct BTShareServer * server, const struct BTACLTransport * transport, struct BTHCIError * error )
{
	struct BTL2CAPLink link;
	struct BTHCIError  cleanup;
	enum BTHCIResult   result;

	uint8_t buffer[8];
	uint8_t event[HCI_EVENT_MAX];
	U32		length;
	U32		reads;

	result = BT_HCICommand ( &transport->HCI, 0x1005, buffer, sizeof ( buffer ), error );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	link.Transport		   = transport;
	link.PacketSize		   = BT_ReadLE16 ( &buffer[1] );
	link.CreditLimit	   = BT_ReadLE16 ( &buffer[4] );
	link.Cancel			   = &server->Request->Cancel;
	link.ExtraReadTimeouts = 0;
	link.PeerMTU		   = BT_L2CAP_MTU;

	if ( ( link.PacketSize < 8 ) || ( ! link.CreditLimit ) )
	{
		return ( BT_HCI_UNSUPPORTED );
	}

	// Page scan lets already-paired peers connect by address.
	result = PageScan ( &transport->HCI, 2, error );

	if ( result == BT_HCI_OK )
	{
		__atomic_store_n ( &server->Request->Listening, 1, __ATOMIC_RELAXED );
	}

	while ( ( result == BT_HCI_OK ) && ( ! BT2_LinkCancelled ( &link ) ) )
	{
		reads = 64;

		result = BT2_ReadEvent ( &transport->HCI, event, &length, &reads, error );

		if ( ( result == BT_HCI_READ_ERROR ) && ( error->TransportError == BT_HCI_TRANSPORT_TIMEOUT ) && ( ! length ) )
		{
			result = BT_HCI_OK;
		}
		else if ( ( result == BT_HCI_OK ) && ( event[0] == 0x04 ) && ( length == 12 ) && ( event[11] == 1 ) )
		{
			result = ServePeer ( server, &link, &event[2], error );
		}
	}

	__atomic_store_n ( &server->Request->Listening, 0, __ATOMIC_RELAXED );

	if ( BT2_LinkCancelled ( &link ) )
	{
		// The USB worker resets the transport after cancellation.
		return ( BT_RFCOMM_CANCELLED );
	}

	PageScan ( &transport->HCI, 0, &cleanup );

	return ( result );
}

// --
