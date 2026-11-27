
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <stdio.h>
#include <string.h>
#include "Prefs.h"

// --

void
Prefs_FormatAddress ( char * text, const uint8_t * address )
{
	snprintf ( text, 18, "%02X:%02X:%02X:%02X:%02X:%02X", (unsigned)address[0], (unsigned)address[1], (unsigned)address[2],
			   (unsigned)address[3], (unsigned)address[4], (unsigned)address[5] );
}

// --

struct PeerRow *
Prefs_Selected ( struct PrefsWindow * gui )
{
	uint32_t index;

	if ( gui->HaveSelection )
	{
		for ( index = 0; index < gui->PeerCount; index++ )
		{
			if ( ! memcmp ( gui->Selection, gui->Peers[index].Address, 6 ) )
			{
				return ( &gui->Peers[index] );
			}
		}
	}

	return ( NULL );
}

// --

static int
MatchesActivePeer ( const struct PrefsWindow * gui, const struct PeerRow * peer )
{
	return ( ( peer ) && ( gui->HaveAdapter ) && ( gui->Adapter.State != BT2_PREFS_IDLE )
			 && ( ! memcmp ( peer->Address, gui->Adapter.Peer, 6 ) ) );
}

// --

void
Prefs_RefreshControls ( struct PrefsWindow * gui )
{
	struct PeerRow * peer;

	int		 available;
	int		 ready;
	int		 idle;
	int		 bonded;
	int		 active;
	int		 disabled[GAD_COUNT];
	uint32_t id;

	peer = Prefs_Selected ( gui );

	available = gui->HaveAdapter && ! gui->Busy;
	ready	  = available && gui->Adapter.Ready;
	idle	  = ready && ! gui->Adapter.Busy && gui->Adapter.State == BT2_PREFS_IDLE;
	bonded	  = peer && peer->Saved;

	active = MatchesActivePeer ( gui, peer );

	disabled[GAD_ADAPTER]	 = gui->Busy || gui->AdapterCount < 2;
	disabled[GAD_PIN]		 = ! idle || ! peer;
	disabled[GAD_SCAN]		 = ! available || gui->Adapter.Busy || gui->Adapter.State != BT2_PREFS_IDLE;
	disabled[GAD_PAIR]		 = ! idle || ! peer || bonded;
	disabled[GAD_CONNECT]	 = ! idle || ! bonded || ! gui->Adapter.HID;
	disabled[GAD_DISCONNECT] = ! ready || ! active || gui->Adapter.State == BT2_PREFS_DISCONNECTING;
	disabled[GAD_FORGET]	 = ! ready || ! bonded || active;
	disabled[GAD_AUTO]		 = ! ready || ! bonded || ! gui->Adapter.HID;

	for ( id = GAD_ADAPTER; id <= GAD_AUTO; id++ )
	{
		if ( id != GAD_DEVICES )
		{
			RefreshSetGadgetAttrs ( (struct Gadget *)gui->Gadgets[id], gui->Window, NULL, GA_Disabled, disabled[id], TAG_END );
		}
	}

	Prefs_RefreshShare ( gui );

	Prefs_RefreshAudio ( gui );
}

// --

static const char *
PeerStatus ( const struct PrefsWindow * gui, const struct PeerRow * peer )
{
	if ( MatchesActivePeer ( gui, peer ) )
	{
		switch ( gui->Adapter.State )
		{
			case BT2_PREFS_CONNECTING:
				return ( "Connecting" );
			case BT2_PREFS_CONNECTED:
				return ( "Connected" );
			case BT2_PREFS_DISCONNECTING:
				return ( "Disconnecting" );
		}
	}

	return ( peer->Saved ? "Paired" : "Found" );
}

// --

static const char *
PeerType ( uint32_t deviceClass )
{
	// Bluetooth Assigned Numbers, Class of Device major and minor fields.
	static const char * const majorTypes[] = {
		"Unknown", "Computer", "Phone", "Network", "Audio / video", "Peripheral", "Imaging", "Wearable", "Toy", "Health device",
	};
	static const char * const audioTypes[] = {
		[1] = "Headset",		[2] = "Hands-free", [4] = "Microphone",	   [5] = "Speaker",	   [6] = "Headphones",
		[7] = "Portable audio", [8] = "Car audio",	[12] = "Video camera", [13] = "Camcorder",
	};
	static const char * const peripheralTypes[] = {
		[1] = "Joystick", [2] = "Gamepad", [3] = "Remote control", [5] = "Digitizer", [7] = "Digital pen", [8] = "Scanner",
	};
	static const char * const inputTypes[] = {
		NULL,
		"Keyboard",
		"Mouse / pointer",
		"Keyboard + mouse",
	};

	uint32_t major;
	uint32_t minor;

	major = ( deviceClass >> 8 ) & 0x1f;
	minor = ( deviceClass >> 2 ) & 0x3f;

	switch ( major )
	{
		case 4:
			if ( ( minor < sizeof ( audioTypes ) / sizeof ( audioTypes[0] ) ) && ( audioTypes[minor] ) )
			{
				return ( audioTypes[minor] );
			}
			break;

		case 5:
			if ( minor >> 4 )
			{
				return ( inputTypes[minor >> 4] );
			}

			if ( ( minor < sizeof ( peripheralTypes ) / sizeof ( peripheralTypes[0] ) ) && ( peripheralTypes[minor] ) )
			{
				return ( peripheralTypes[minor] );
			}
			break;

		case 6:
			switch ( minor )
			{
				case 4:
					return ( "Display" );
				case 8:
					return ( "Camera" );
				case 16:
					return ( "Scanner" );
				case 32:
					return ( "Printer" );
			}
			break;
	}

	return ( major < sizeof ( majorTypes ) / sizeof ( majorTypes[0] ) ? majorTypes[major] : "Unknown" );
}

// --

void
Prefs_RebuildRows ( struct PrefsWindow * gui, int reload )
{
	struct BT2PrefsPeer saved[PREFS_PEERS];
	struct PeerRow *	peer;
	struct Node *		row;
	struct Node *		selected;

	uint32_t count;
	uint32_t index;
	uint32_t known;
	char	 address[18];

	selected = NULL;

	if ( reload )
	{
		gui->PeerCount = 0;

		if ( gui->Adapter.Ready
			 && gui->Bluetooth->prefs_Peers ( gui->Adapter.Address, saved, PREFS_PEERS, &count ) == BT_HCI_OK )
		{
			for ( index = 0; ( index < count ) && ( index < PREFS_PEERS ); index++ )
			{
				peer  = &gui->Peers[gui->PeerCount++];
				*peer = (struct PeerRow){ .Saved = 1, .Auto = saved[index].AutoConnect };

				memcpy ( peer->Address, saved[index].Address, 6 );
			}
		}

		for ( index = 0; ( index < gui->Reports ) && ( index < PREFS_PEERS ); index++ )
		{
			for ( known = 0; known < gui->PeerCount; known++ )
			{
				if ( ! memcmp ( gui->Peers[known].Address, gui->Job.Nearby[index].Address, 6 ) )
				{
					break;
				}
			}

			peer = &gui->Peers[known];

			if ( known == gui->PeerCount )
			{
				gui->PeerCount++;
				*peer = (struct PeerRow){ 0 };

				memcpy ( peer->Address, gui->Job.Nearby[index].Address, 6 );
			}

			peer->Class = gui->Job.Nearby[index].ClassOfDevice;
		}
	}

	SetAttrs ( gui->Gadgets[GAD_DEVICES], LISTBROWSER_Labels, ~0UL, TAG_END );

	gui->ListBrowser->FreeListBrowserList ( &gui->Rows );

	for ( index = 0; index < gui->PeerCount; index++ )
	{
		peer = &gui->Peers[index];

		Prefs_FormatAddress ( address, peer->Address );

		row = gui->ListBrowser->AllocListBrowserNode ( 4, LBNA_UserData, peer, LBNA_Column, 0, LBNCA_CopyText, TRUE, LBNCA_Text,
													   address, LBNA_Column, 1, LBNCA_Text, PeerType ( peer->Class ),
													   LBNA_Column, 2, LBNCA_Text, PeerStatus ( gui, peer ), LBNA_Column, 3,
													   LBNCA_Text, peer->Auto ? "Yes" : "", TAG_END );

		if ( ! row )
		{
			break;
		}

		AddTail ( &gui->Rows, row );

		if ( ( gui->HaveSelection ) && ( ! memcmp ( gui->Selection, peer->Address, 6 ) ) )
		{
			selected = row;
		}
	}

	gui->HaveSelection = selected != NULL;

	RefreshSetGadgetAttrs ( (struct Gadget *)gui->Gadgets[GAD_DEVICES], gui->Window, NULL, LISTBROWSER_Labels, &gui->Rows,
							LISTBROWSER_SelectedNode, selected, TAG_END );

	Prefs_RefreshControls ( gui );
}

// --
