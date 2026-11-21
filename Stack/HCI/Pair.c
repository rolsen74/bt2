
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <stddef.h>
#include "bt2_hci.h"

// --

#define HCI_CREATE_CONNECTION 0x0405
#define HCI_DISCONNECT 0x0406
#define HCI_LINK_KEY_REPLY 0x040b
#define HCI_LINK_KEY_NEGATIVE_REPLY 0x040c
#define HCI_PIN_CODE_REPLY 0x040d
#define HCI_PIN_CODE_NEGATIVE_REPLY 0x040e
#define HCI_AUTHENTICATION_REQUESTED 0x0411
#define HCI_SET_CONNECTION_ENCRYPTION 0x0413

#define HCI_EVENT_CONNECTION_COMPLETE 0x03
#define HCI_EVENT_DISCONNECTION_COMPLETE 0x05
#define HCI_EVENT_AUTHENTICATION_COMPLETE 0x06
#define HCI_EVENT_ENCRYPTION_CHANGE 0x08
#define HCI_EVENT_PIN_CODE_REQUEST 0x16
#define HCI_EVENT_LINK_KEY_REQUEST 0x17
#define HCI_EVENT_LINK_KEY_NOTIFICATION 0x18
#define HCI_DEBUG_COMBINATION_KEY 3

// --

static uint16_t
ReadLE16( const uint8_t *bytes )
{
	return ( bytes[0] | ( (uint16_t) bytes[1] << 8 ) );
}

// --

static int
MatchesAddress( const struct BTPairRequest *pair, const uint8_t *wire )
{
	uint32_t byte;

	for( byte = 0; byte < sizeof( pair->Address ); byte++ )
	{
		if ( wire[byte] != pair->Address[5 - byte] )
		{
			return ( 0 );
		}
	}

	return ( 1 );
}

// --

static enum BTHCIResult
SendPairCommand( const struct BTHCITransport *transport, uint16_t opcode,
	uint8_t *command, uint8_t length, struct BTHCIError *error )
{
	command[0] = opcode & 0xff;
	command[1] = opcode >> 8;
	command[2] = length;
	error->Opcode = opcode;
	error->Status = 0;

	error->TransportError = transport->StartCommand( transport->Context, command, length + 3U );

	return ( error->TransportError ? BT_HCI_WRITE_ERROR : BT_HCI_OK );
}

// --

static enum BTHCIResult
ReplyToPairRequest( const struct BTHCITransport *transport, const struct BTPairRequest *pair,
	struct BTPairKeys *keys, const uint8_t *event, struct BTHCIError *error )
{
	enum BTHCIResult result;

	uint8_t command[3 + 6 + 1 + BT_PIN_MAX];
	uint8_t length;
	uint16_t opcode;
	uint32_t byte;

	for( byte = 0; byte < 6; byte++ )
	{
		command[3 + byte] = event[2 + byte];
	}

	length = 6;

	if ( event[0] == HCI_EVENT_PIN_CODE_REQUEST )
	{
		opcode = HCI_PIN_CODE_NEGATIVE_REPLY;

		if ( pair->PINLength )
		{
			opcode = HCI_PIN_CODE_REPLY;
			length = 7 + BT_PIN_MAX;
			command[9] = pair->PINLength;

			for( byte = 0; byte < BT_PIN_MAX; byte++ )
			{
				command[10 + byte] = byte < pair->PINLength ? pair->PIN[byte] : 0;
			}
		}
	}
	else
	{
		opcode = HCI_LINK_KEY_NEGATIVE_REPLY;

		if ( ( keys ) && ( keys->Saved ) )
		{
			opcode = HCI_LINK_KEY_REPLY;
			length = 6 + BT_LINK_KEY_SIZE;

			for( byte = 0; byte < BT_LINK_KEY_SIZE; byte++ )
			{
				command[9 + byte] = keys->Saved[byte];
			}
		}
	}

	result = SendPairCommand( transport, opcode, command, length, error );

	for( byte = 9; byte < sizeof( command ); byte++ )
	{
		((volatile uint8_t *)command)[byte] = 0;
	}

	if ( ( result == BT_HCI_OK ) && ( opcode == HCI_LINK_KEY_REPLY ) )
	{
		keys->Sent = 1;
	}

	return ( result );
}

// --

static enum BTHCIResult
WaitPairEvent( const struct BTHCITransport *transport, const struct BTPairRequest *pair,
	struct BTPairKeys *keys, uint8_t expected, uint16_t *handle, struct BTHCIError *error )
{
	enum BTHCIResult result;

	uint8_t event[HCI_EVENT_MAX];
	uint32_t length;
	uint32_t reads;
	uint32_t timeouts;
	uint32_t timeoutLimit;
	uint32_t byte;
	uint16_t operation;
	uint16_t opcode;

	operation = error->Opcode;
	reads = 256;
	timeouts = 0;
	timeoutLimit = expected == HCI_EVENT_DISCONNECTION_COMPLETE ? 4 : 24;

	while( reads )
	{
		result = BT2_ReadEvent( transport, event, & length, & reads, error );

		if ( result != BT_HCI_OK )
		{
			if ( ( result == BT_HCI_READ_ERROR ) && ( error->TransportError == BT_HCI_TRANSPORT_TIMEOUT ) &&
				( ! length ) && ( ++timeouts < timeoutLimit ) )
			{
				continue;
			}

			return ( result );
		}

		if ( ( event[0] == HCI_EVENT_COMMAND_STATUS ) && ( length == 6 ) &&
			( ReadLE16( & event[4] ) == operation ) && ( event[2] ) )
		{
			error->Status = event[2];

			return ( BT_HCI_COMMAND_ERROR );
		}

		if ( ( event[0] == HCI_EVENT_COMMAND_COMPLETE ) && ( length >= 6 ) )
		{
			opcode = ReadLE16( & event[3] );

			if ( ( ( opcode >= HCI_LINK_KEY_REPLY ) && ( opcode <= HCI_PIN_CODE_NEGATIVE_REPLY ) ) && ( event[5] ) )
			{
				error->Opcode = opcode;
				error->Status = event[5];

				return ( BT_HCI_COMMAND_ERROR );
			}
		}

		if ( ( event[0] == HCI_EVENT_LINK_KEY_NOTIFICATION ) && ( length == 25 ) )
		{
			if ( ( keys ) && ( ! keys->Saved ) && ( event[24] != HCI_DEBUG_COMBINATION_KEY ) &&
				( MatchesAddress( pair, & event[2] ) ) )
			{
				for( byte = 0; byte < BT_LINK_KEY_SIZE; byte++ )
				{
					keys->New[byte] = event[8 + byte];
				}

				keys->NewValid = 1;
			}

			for( byte = 0; byte < BT_LINK_KEY_SIZE; byte++ )
			{
				((volatile uint8_t *)event)[8 + byte] = 0;
			}

			continue;
		}

		if ( ( expected != HCI_EVENT_DISCONNECTION_COMPLETE ) && ( length == 8 ) &&
			( ( event[0] == HCI_EVENT_PIN_CODE_REQUEST ) || ( event[0] == HCI_EVENT_LINK_KEY_REQUEST ) ) &&
			( MatchesAddress( pair, & event[2] ) ) )
		{
			result = ReplyToPairRequest( transport, pair, keys, event, error );

			if ( result != BT_HCI_OK )
			{
				return ( result );
			}

			error->Opcode = operation;

			continue;
		}

		if ( ( event[0] == HCI_EVENT_DISCONNECTION_COMPLETE ) && ( length == 6 ) &&
			( ReadLE16( & event[3] ) == *handle ) && ( expected != HCI_EVENT_DISCONNECTION_COMPLETE ) )
		{
			error->Status = event[5];

			return ( BT_HCI_COMMAND_ERROR );
		}

		if ( event[0] != expected )
		{
			continue;
		}

		if ( expected == HCI_EVENT_CONNECTION_COMPLETE )
		{
			if ( ( length != 13 ) || ( ! MatchesAddress( pair, & event[5] ) ) )
			{
				continue;
			}

			if ( ! event[2] )
			{
				*handle = ReadLE16( & event[3] );

				if ( event[11] != 1 )
				{
					return ( BT_HCI_INVALID_REPLY );
				}
			}
		}
		else
		{
			if ( ( length != ( expected == HCI_EVENT_AUTHENTICATION_COMPLETE ? 5U : 6U ) ) ||
				( ReadLE16( & event[3] ) != *handle ) )
			{
				continue;
			}

			if ( ( expected == HCI_EVENT_ENCRYPTION_CHANGE ) && ( ! event[2] ) && ( ! event[5] ) )
			{
				return ( BT_HCI_INVALID_REPLY );
			}
		}

		error->Status = event[2];

		return ( error->Status ? BT_HCI_COMMAND_ERROR : BT_HCI_OK );
	}

	return ( BT_HCI_INVALID_REPLY );
}

// --

enum BTHCIResult
BT2_OpenClassicLink( const struct BTHCITransport *transport, const struct BTPairRequest *pair,
	struct BTPairKeys *keys, uint16_t packetTypes, uint16_t *handle, struct BTHCIError *error )
{
	enum BTHCIResult result;

	uint8_t command[16];
	uint32_t byte;

	for( byte = 0; byte < sizeof( pair->Address ); byte++ )
	{
		command[3 + byte] = pair->Address[5 - byte];
	}

	// Keep role switching and cached clock offset disabled.
	command[9] = packetTypes & 0xff;
	command[10] = packetTypes >> 8;
	command[11] = 2;
	command[12] = 0;
	command[13] = 0;
	command[14] = 0;
	command[15] = 0;

	result = SendPairCommand( transport, HCI_CREATE_CONNECTION, command, 13, error );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	return ( WaitPairEvent( transport, pair, keys, HCI_EVENT_CONNECTION_COMPLETE, handle, error ) );
}

// --

static enum BTHCIResult
LinkCommand( const struct BTHCITransport *transport, const struct BTPairRequest *pair,
	struct BTPairKeys *keys, uint16_t opcode,
	uint8_t parameter, uint8_t expected, uint16_t *handle, struct BTHCIError *error )
{
	enum BTHCIResult result;

	uint8_t command[6];

	command[3] = *handle & 0xff;
	command[4] = *handle >> 8;
	command[5] = parameter;

	result = SendPairCommand( transport, opcode, command, opcode == HCI_AUTHENTICATION_REQUESTED ? 2 : 3, error );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	return ( WaitPairEvent( transport, pair, keys, expected, handle, error ) );
}

// --

enum BTHCIResult
BT2_CloseClassicLink( const struct BTHCITransport *transport, const struct BTPairRequest *pair,
	struct BTPairKeys *keys, uint16_t handle, struct BTHCIError *error )
{
	struct BTHCIError resetError;
	enum BTHCIResult result;

	if ( handle == BT_CLASSIC_INVALID_HANDLE )
	{
		// An incomplete Create Connection may still have paging in progress.
		return ( BT_ResetController( transport, error ) );
	}

	result = LinkCommand( transport, pair, keys, HCI_DISCONNECT, 0x13, HCI_EVENT_DISCONNECTION_COMPLETE, & handle, error );

	if ( result != BT_HCI_OK )
	{
		BT_ResetController( transport, & resetError );
	}

	return ( result );
}

// --

enum BTHCIResult
BT2_AuthenticateClassicLink( const struct BTHCITransport *transport, const struct BTPairRequest *pair,
	struct BTPairKeys *keys, uint16_t handle, struct BTHCIError *error )
{
	enum BTHCIResult result;

	result = LinkCommand( transport, pair, keys, HCI_AUTHENTICATION_REQUESTED, 0,
		HCI_EVENT_AUTHENTICATION_COMPLETE, & handle, error );

	if ( result == BT_HCI_OK )
	{
		result = LinkCommand( transport, pair, keys, HCI_SET_CONNECTION_ENCRYPTION, 1,
			HCI_EVENT_ENCRYPTION_CHANGE, & handle, error );
	}

	return ( result );
}

// --

enum BTHCIResult
BT2_PairWithKeys( const struct BTHCITransport *transport, const struct BTPairRequest *pair,
	struct BTPairKeys *keys, struct BTHCIError *error )
{
	struct BTHCIError cleanupError;
	enum BTHCIResult result;
	enum BTHCIResult cleanupResult;

	uint16_t handle;

	if ( ( ! pair ) || ( pair->PINLength > BT_PIN_MAX ) ||
		( ( ! pair->PINLength ) && ( ( ! keys ) || ( ! keys->Saved ) ) ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	handle = BT_CLASSIC_INVALID_HANDLE;

	result = BT2_OpenClassicLink( transport, pair, keys, BT_ACL_PACKETS_DM1, & handle, error );

	if ( result == BT_HCI_OK )
	{
		result = BT2_AuthenticateClassicLink( transport, pair, keys, handle, error );
	}

	cleanupResult = BT2_CloseClassicLink( transport, pair, keys, handle, & cleanupError );

	if ( ( result == BT_HCI_OK ) && ( cleanupResult != BT_HCI_OK ) )
	{
		*error = cleanupError;

		return ( cleanupResult );
	}

	return ( result );
}

// --

enum BTHCIResult
BT_Pair( const struct BTHCITransport *transport, const struct BTPairRequest *pair, struct BTHCIError *error )
{
	return ( BT2_PairWithKeys( transport, pair, NULL, error ) );
}

// --
