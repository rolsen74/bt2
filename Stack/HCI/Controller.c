
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "Bluetooth.h"

// --

#define HCI_RESET			   0x0c03
#define HCI_READ_LOCAL_VERSION 0x1001
#define HCI_READ_BD_ADDR	   0x1009

// --

enum BTHCIResult
BT_ResetController ( const struct BTHCITransport * transport, struct BTHCIError * error )
{
	uint8_t reply[1];

	return ( BT_HCICommand ( transport, HCI_RESET, reply, sizeof ( reply ), error ) );
}

// --

enum BTHCIResult
BT_ReadControllerInfo ( const struct BTHCITransport * transport, struct BTControllerInfo * info, struct BTHCIError * error )
{
	enum BTHCIResult result;

	uint8_t	 version[9];
	uint8_t	 address[7];
	uint32_t byte;

	result = BT_HCICommand ( transport, HCI_READ_LOCAL_VERSION, version, sizeof ( version ), error );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	result = BT_HCICommand ( transport, HCI_READ_BD_ADDR, address, sizeof ( address ), error );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	info->HCIVersion	= version[1];
	info->HCIRevision	= ( (uint16_t)version[3] << 8 ) | version[2];
	info->LMPVersion	= version[4];
	info->Manufacturer	= ( (uint16_t)version[6] << 8 ) | version[5];
	info->LMPSubversion = ( (uint16_t)version[8] << 8 ) | version[7];

	for ( byte = 0; byte < sizeof ( info->Address ); byte++ )
	{
		info->Address[byte] = address[sizeof ( info->Address ) - byte];
	}

	return ( BT_HCI_OK );
}

// --
