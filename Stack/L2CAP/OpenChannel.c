
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_l2cap.h"

// --

#define L2CAP_CONNECT_REQ 0x02
#define L2CAP_CONNECT_RSP 0x03
#define L2CAP_CONFIG_REQ  0x04
#define L2CAP_CONFIG_RSP  0x05
#define L2CAP_CONNECT_ID  1
#define L2CAP_CONFIG_ID	  2

// --

enum BTHCIResult
BT2_ConfigurePeer ( struct BTL2CAPLink * link, const struct BTL2CAPSignal * signal, uint16_t remoteCID,
					struct BTHCIError * error )
{
	const uint8_t * option;
	uint8_t			response[6];
	uint32_t		remaining;
	uint32_t		length;
	uint8_t			kind;

	remaining = signal->Length - 4;
	option	  = signal->Data + 4;

	if ( BT_ReadLE16 ( signal->Data + 2 ) )
	{
		return ( BT_HCI_UNSUPPORTED );
	}

	while ( remaining )
	{
		if ( ( remaining < 2 ) || ( option[1] > remaining - 2 ) )
		{
			return ( BT_HCI_INVALID_REPLY );
		}

		kind   = option[0] & 0x7f;
		length = option[1];

		switch ( kind )
		{
			case 1: // MTU.
				if ( ( length != 2 ) || ( BT_ReadLE16 ( option + 2 ) < 48 ) )
				{
					return ( BT_HCI_INVALID_REPLY );
				}

				link->PeerMTU = BT_ReadLE16 ( option + 2 );

				break;

			case 2: // Flush timeout.
				if ( length != 2 )
				{
					return ( BT_HCI_INVALID_REPLY );
				}

				break;

			case 4: // Retransmission and Flow Control mode.
				if ( ( length != 9 ) || ( option[2] ) )
				{
					return ( BT_HCI_UNSUPPORTED );
				}

				break;

			default:
				if ( ! ( option[0] & 0x80 ) )
				{
					return ( BT_HCI_UNSUPPORTED );
				}

				break;
		}

		remaining -= 2 + length;
		option += 2 + length;
	}

	response[0] = remoteCID & 0xff;
	response[1] = remoteCID >> 8;
	response[2] = 0;
	response[3] = 0;
	response[4] = 0;
	response[5] = 0;

	return ( BT2_WriteSignal ( link, L2CAP_CONFIG_RSP, signal->ID, response, sizeof ( response ), error ) );
}

// --

enum BTHCIResult
BT2_OpenChannel ( struct BTL2CAPLink * link, uint16_t psm, uint16_t localCID, uint16_t * remoteCID, struct BTHCIError * error )
{
	struct BTL2CAPPacket pdu;
	struct BTL2CAPSignal signal;
	enum BTHCIResult	 result;

	const uint8_t * cursor;
	uint8_t			parameters[4];
	uint32_t		remaining;
	uint32_t		reads;
	uint16_t		status;
	int				localReady;
	int				peerReady;

	*remoteCID	  = 0;
	localReady	  = 0;
	peerReady	  = 0;
	reads		  = 256;
	parameters[0] = psm & 0xff;
	parameters[1] = psm >> 8;
	parameters[2] = localCID & 0xff;
	parameters[3] = localCID >> 8;

	result = BT2_WriteSignal ( link, L2CAP_CONNECT_REQ, L2CAP_CONNECT_ID, parameters, sizeof ( parameters ), error );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	while ( reads )
	{
		result = BT2_ReadL2CAP ( link, &pdu, &reads, error );

		if ( result != BT_HCI_OK )
		{
			return ( result );
		}

		if ( pdu.CID != BT_L2CAP_SIGNAL_CID )
		{
			continue;
		}

		cursor	  = pdu.Data + BT_L2CAP_HEADER;
		remaining = pdu.Length;

		while ( BT2_DecodeSignal ( &cursor, &remaining, &signal ) )
		{
			if ( ( signal.Code == L2CAP_CONNECT_RSP ) && ( signal.ID == L2CAP_CONNECT_ID ) && ( signal.Length == 8 )
				 && ( BT_ReadLE16 ( signal.Data + 2 ) == localCID ) && ( ! *remoteCID ) )
			{
				status = BT_ReadLE16 ( signal.Data + 4 );

				if ( status == 1 ) // Pending.
				{
					continue;
				}

				if ( status )
				{
					error->Status = status;

					return ( BT_L2CAP_REJECTED );
				}

				*remoteCID = BT_ReadLE16 ( signal.Data );

				if ( *remoteCID < 0x0040 )
				{
					return ( BT_HCI_INVALID_REPLY );
				}

				parameters[0] = *remoteCID & 0xff;
				parameters[1] = *remoteCID >> 8;
				parameters[2] = 0;
				parameters[3] = 0;

				// Empty options request Basic Mode and the default 672-byte MTU.
				result = BT2_WriteSignal ( link, L2CAP_CONFIG_REQ, L2CAP_CONFIG_ID, parameters, sizeof ( parameters ), error );
			}
			else if ( ( signal.Code == L2CAP_CONFIG_REQ ) && ( signal.ID ) && ( signal.Length >= 4 ) && ( *remoteCID )
					  && ( BT_ReadLE16 ( signal.Data ) == localCID ) )
			{
				result = BT2_ConfigurePeer ( link, &signal, *remoteCID, error );

				peerReady = result == BT_HCI_OK;
			}
			else if ( ( signal.Code == L2CAP_CONFIG_RSP ) && ( signal.ID == L2CAP_CONFIG_ID ) && ( signal.Length >= 6 )
					  && ( *remoteCID ) && ( BT_ReadLE16 ( signal.Data ) == localCID ) )
			{
				status = BT_ReadLE16 ( signal.Data + 4 );

				if ( status == 4 ) // Pending.
				{
					continue;
				}

				if ( status )
				{
					error->Status = status;

					return ( BT_L2CAP_REJECTED );
				}

				if ( BT_ReadLE16 ( signal.Data + 2 ) )
				{
					return ( BT_HCI_UNSUPPORTED );
				}

				localReady = 1;
			}
			else if ( ( signal.Code == BT_L2CAP_SIGNAL_REJECT ) && ( signal.Length >= 2 )
					  && ( signal.ID == ( *remoteCID ? L2CAP_CONFIG_ID : L2CAP_CONNECT_ID ) ) )
			{
				error->Status = BT_ReadLE16 ( signal.Data );

				return ( BT_L2CAP_REJECTED );
			}
			else
			{
				result = BT2_ServiceSignal ( link, &signal, error );
			}

			if ( result != BT_HCI_OK )
			{
				return ( result );
			}
		}

		if ( ( localReady ) && ( peerReady ) )
		{
			return ( BT_HCI_OK );
		}
	}

	return ( BT_HCI_INVALID_REPLY );
}

// --
