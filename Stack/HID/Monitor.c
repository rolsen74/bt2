
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <stddef.h>
#include "bt2_hid.h"

// --

#define HID_CONTROL_PSM	  0x0011
#define HID_INTERRUPT_PSM 0x0013
#define HID_CONTROL_CID	  0x0040
#define HID_INTERRUPT_CID 0x0041

struct BTHIDLink
{
	struct BTL2CAPLink ACL;
	uint16_t		   Control;
	uint16_t		   Interrupt;
};

// --

static enum BTHCIResult
ReadHIDPacket ( struct BTHIDLink * hid, struct BTL2CAPPacket * packet, struct BTHCIError * error )
{
	struct BTL2CAPSignal signal;
	enum BTHCIResult	 result;

	const uint8_t * data;
	uint32_t		remaining;
	uint32_t		reads;
	uint16_t		destination;
	uint16_t		source;

	reads = 64;

	result = BT2_ReadL2CAPWithTimeouts ( &hid->ACL, packet, &reads, 1, error );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	data = packet->Data + BT_L2CAP_HEADER;

	if ( ( packet->CID == HID_CONTROL_CID ) && ( packet->Length == 1 ) && ( data[0] == 0x15 ) )
	{
		return ( BT_HID_DISCONNECTED ); // Virtual Cable Unplug.
	}

	if ( packet->CID == BT_L2CAP_SIGNAL_CID )
	{
		remaining = packet->Length;

		while ( BT2_DecodeSignal ( &data, &remaining, &signal ) )
		{
			if ( ( signal.ID ) && ( signal.Code == 6 ) && ( signal.Length == 4 ) )
			{
				destination = BT_ReadLE16 ( signal.Data );

				source = BT_ReadLE16 ( signal.Data + 2 );

				if ( ( ( destination == HID_CONTROL_CID ) && ( source == hid->Control ) )
					 || ( ( destination == HID_INTERRUPT_CID ) && ( source == hid->Interrupt ) ) )
				{
					result = BT2_WriteSignal ( &hid->ACL, 7, signal.ID, signal.Data, 4, error );

					return ( result == BT_HCI_OK ? BT_HID_DISCONNECTED : result );
				}
			}

			result = BT2_ServiceSignal ( &hid->ACL, &signal, error );

			if ( result != BT_HCI_OK )
			{
				return ( result );
			}
		}
	}

	return ( BT_HCI_OK );
}

// --

static int
IdleTimeout ( enum BTHCIResult result, const struct BTHCIError * error )
{
	return ( ( result == BT_ACL_READ_ERROR ) && ( error->TransportError == BT_HCI_TRANSPORT_TIMEOUT ) );
}

// --

static enum BTHCIResult
SetBootProtocol ( struct BTHIDLink * hid, struct BTHIDMonitor * monitor, struct BTHCIError * error )
{
	struct BTL2CAPPacket packet;
	enum BTHCIResult	 result;

	uint8_t	 setProtocol;
	uint8_t	 response;
	uint32_t count;
	uint32_t timeouts;

	setProtocol = 0x70; // SET_PROTOCOL, Boot mode.
	timeouts	= 0;

	result = BT2_WriteL2CAP ( &hid->ACL, hid->Control, &setProtocol, 1, error );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	for ( count = 0; count < 64; count++ )
	{
		result = ReadHIDPacket ( hid, &packet, error );

		if ( ( IdleTimeout ( result, error ) ) && ( ++timeouts < 6 ) )
		{
			continue;
		}

		if ( result != BT_HCI_OK )
		{
			return ( result );
		}

		if ( ( packet.CID == HID_CONTROL_CID ) && ( packet.Length == 1 ) )
		{
			response = packet.Data[BT_L2CAP_HEADER];

			if ( ( response & 0xf0 ) == 0 )
			{
				monitor->Handshake = response;

				return ( response ? BT_HID_REJECTED : BT_HCI_OK );
			}
		}
	}

	return ( result == BT_HCI_OK ? BT_HCI_INVALID_REPLY : result );
}

// --

static int
DecodeReport ( struct BTHIDReport * report, const struct BTL2CAPPacket * packet )
{
	const uint8_t * data;
	uint32_t		length;
	uint32_t		byte;
	uint8_t			id;

	if ( ( packet->CID != HID_INTERRUPT_CID ) || ( packet->Length < 2 ) || ( packet->Length > 2 + BT_HID_BOOT_PAYLOAD_MAX ) )
	{
		return ( 0 );
	}

	data   = packet->Data + BT_L2CAP_HEADER;
	id	   = data[1];
	length = packet->Length - 2;

	if ( ( data[0] != 0xa1 )
		 || ( ! ( ( ( id == BT_HID_BOOT_KEYBOARD ) && ( length >= 8 ) )
				  || ( ( id == BT_HID_BOOT_MOUSE ) && ( length >= 3 ) ) ) ) )
	{
		return ( 0 );
	}

	report->ID	   = id;
	report->Length = length;

	for ( byte = 0; byte < length; byte++ )
	{
		report->Data[byte] = data[2 + byte];
	}

	return ( 1 );
}

// --

static enum BTHCIResult
OpenHIDChannels ( struct BTHIDLink * hid, struct BTHIDMonitor * monitor, const char ** phase, struct BTHCIError * error )
{
	enum BTHCIResult result;

	hid->ACL.Cancel = &monitor->Cancel;

	*phase = "control channel setup";

	result = BT2_OpenChannel ( &hid->ACL, HID_CONTROL_PSM, HID_CONTROL_CID, &hid->Control, error );

	if ( result == BT_HCI_OK )
	{
		*phase = "interrupt channel setup";

		result = BT2_OpenChannel ( &hid->ACL, HID_INTERRUPT_PSM, HID_INTERRUPT_CID, &hid->Interrupt, error );
	}

	if ( result == BT_HCI_OK )
	{
		*phase = "boot protocol negotiation";

		result = SetBootProtocol ( hid, monitor, error );
	}

	return ( result );
}

// --

enum BTHCIResult
BT2_MonitorHID ( const struct BTACLTransport * transport, struct BTHIDMonitor * monitor, const uint8_t * key,
				 int ( *expired ) ( void * context ), void * context, struct BTHCIError * error )
{
	struct BTHIDLink	 hid;
	struct BTL2CAPPacket packet;
	enum BTHCIResult	 result;

	const char * phase;

	if ( ( ! monitor ) || ( ! monitor->Reports ) || ( ! monitor->Capacity ) || ( ! expired ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	monitor->Count	   = 0;
	monitor->Handshake = 0xff;

	if ( __atomic_load_n ( &monitor->Cancel, __ATOMIC_RELAXED ) )
	{
		return ( BT_RFCOMM_CANCELLED );
	}

	result = BT2_OpenDataLinkWithKey ( &hid.ACL, transport, monitor->Address, key, error );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	result = OpenHIDChannels ( &hid, monitor, &phase, error );

	while ( ( result == BT_HCI_OK ) && ( monitor->Count < monitor->Capacity ) && ( ! expired ( context ) ) )
	{
		result = ReadHIDPacket ( &hid, &packet, error );

		if ( IdleTimeout ( result, error ) )
		{
			result = BT_HCI_OK;
		}
		else if ( ( result == BT_HCI_OK ) && ( DecodeReport ( &monitor->Reports[monitor->Count], &packet ) ) )
		{
			monitor->Count++;
		}
	}

	return ( BT2_CloseDataLink ( &hid.ACL, result, error ) );
}

// --

enum BTHCIResult
BT2_ConnectHIDInput ( const struct BTACLTransport * transport, struct BTHIDInput * input, const uint8_t * key,
					  const char ** phase, struct BTHCIError * error )
{
	struct BTHIDLink	 hid;
	struct BTL2CAPPacket packet;
	struct BTHIDReport	 report;
	enum BTHCIResult	 result;

	int poll;

	if ( __atomic_load_n ( &input->Monitor.Cancel, __ATOMIC_RELAXED ) )
	{
		return ( BT_RFCOMM_CANCELLED );
	}

	result = BT2_OpenDataLinkWithKey ( &hid.ACL, transport, input->Monitor.Address, key, error );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	result = OpenHIDChannels ( &hid, &input->Monitor, phase, error );

	if ( result == BT_HCI_OK )
	{
		*phase = NULL;

		if ( ! input->Start ( input->Context ) )
		{
			result = BT_HID_INPUT_ERROR;
		}
		else
		{
			poll = 1;

			while ( result == BT_HCI_OK )
			{
				result = ReadHIDPacket ( &hid, &packet, error );

				if ( IdleTimeout ( result, error ) )
				{
					result = poll ? BT2_CheckHIDConnection ( &hid.ACL, error ) : BT_HCI_OK;

					if ( result == BT_HCI_UNSUPPORTED )
					{
						poll   = 0;
						result = BT_HCI_OK;
					}
				}
				else if ( ( result == BT_HCI_OK ) && ( DecodeReport ( &report, &packet ) ) )
				{
					if ( ! input->Report ( input->Context, &report ) )
					{
						result = BT_HID_INPUT_ERROR;
					}
					else
					{
						input->Monitor.Count++;
					}
				}
			}

			input->Stop ( input->Context );
		}
	}

	return ( BT2_CloseDataLink ( &hid.ACL, result, error ) );
}

// --
