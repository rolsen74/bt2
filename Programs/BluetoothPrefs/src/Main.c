
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <stdio.h>
#include <string.h>
#include <devices/timer.h>
#include <dos/var.h>
#include "Prefs.h"

// --

static const char Version[] __attribute__ ( ( used ) ) = "\0$VER: " PREFS_VERSION;

// --

static void
UpdateAdapterChooser ( struct PrefsWindow * gui, const struct BT2ManagedAdapter * adapters, uint32_t count, uint32_t selected )
{
	uint32_t index;
	int		 changed;
	ULONG	 current;
	char	 address[18];
	char	 label[48];

	changed			  = gui->AdapterCount != count;
	gui->AdapterCount = count;

	for ( index = 0; index < count; index++ )
	{
		if ( adapters[index].Adapter.Ready )
		{
			Prefs_FormatAddress ( address, adapters[index].Adapter.Address );

			snprintf ( label, sizeof ( label ), "Unit %lu   %s", (unsigned long)adapters[index].Adapter.Unit, address );
		}
		else
		{
			snprintf ( label, sizeof ( label ), "Unit %lu   Address %s", (unsigned long)adapters[index].Adapter.Unit,
					   adapters[index].Adapter.LastResult == BT_HCI_OK ? "pending" : "unavailable" );
		}

		if ( strcmp ( gui->AdapterText[index], label ) )
		{
			strcpy ( gui->AdapterText[index], label );

			changed = TRUE;
		}

		gui->AdapterLabels[index] = gui->AdapterText[index];
	}

	gui->AdapterLabels[count] = NULL;

	if ( ! count )
	{
		gui->AdapterLabels[0] = "No adapter";
		gui->AdapterLabels[1] = NULL;
	}

	current = selected;

	GetAttr ( CHOOSER_Selected, gui->Gadgets[GAD_ADAPTER], &current );

	if ( ( changed ) || ( current != selected ) )
	{
		RefreshSetGadgetAttrs ( (struct Gadget *)gui->Gadgets[GAD_ADAPTER], gui->Window, NULL, CHOOSER_LabelArray,
								gui->AdapterLabels, CHOOSER_Selected, selected, TAG_END );

		Prefs_RefreshControls ( gui );
	}
}

// --

static void
RefreshAdapter ( struct PrefsWindow * gui, int choose )
{
	struct BT2ManagedAdapter adapters[PREFS_ADAPTERS];
	struct BT2PrefsAdapter * adapter;

	uint32_t count;
	uint32_t index;
	ULONG	 selected;
	int		 changed;
	int		 reload;

	memset ( adapters, 0, sizeof ( adapters ) );

	count = gui->Bluetooth->control_Adapters ( adapters, PREFS_ADAPTERS );

	if ( count > PREFS_ADAPTERS )
	{
		count = PREFS_ADAPTERS;
	}

	if ( ! count )
	{
		UpdateAdapterChooser ( gui, adapters, 0, 0 );

		if ( gui->HaveAdapter )
		{
			gui->HaveAdapter  = FALSE;
			gui->Registration = 0;

			Prefs_SetStatus ( gui, "No Bluetooth controller registered. Check the USB Bluetooth driver." );

			Prefs_RebuildRows ( gui, FALSE );
		}

		return;
	}

	for ( index = 0; index < count; index++ )
	{
		if ( adapters[index].Registration == gui->Registration )
		{
			break;
		}
	}

	if ( choose )
	{
		selected = index;

		GetAttr ( CHOOSER_Selected, gui->Gadgets[GAD_ADAPTER], &selected );

		index = selected;
	}

	if ( index >= count )
	{
		index = 0;
	}

	UpdateAdapterChooser ( gui, adapters, count, index );

	adapter = &adapters[index].Adapter;
	changed = ( adapters[index].Registration != gui->Registration )
			  || ( ( adapter->Ready ) && ( gui->Adapter.Ready ) && memcmp ( adapter->Address, gui->Adapter.Address, 6 ) );
	reload	= adapter->Ready && ( changed || ! gui->Adapter.Ready );

	if ( ( ! gui->Busy ) && ( ! gui->Sharing ) && ( ! gui->HaveAdapter || changed || ( adapter->Ready != gui->Adapter.Ready ) ) )
	{
		Prefs_SetStatus ( gui, adapter->Ready ? "Controller ready." : "Controller registered. Reading its Bluetooth address." );
	}

	if ( changed )
	{
		gui->Reports	   = 0;
		gui->HaveSelection = FALSE;
	}

	if ( ! gui->HaveAdapter || changed || memcmp ( adapter, &gui->Adapter, sizeof ( *adapter ) ) )
	{
		if ( ( ! gui->Busy ) && ( ! gui->Sharing ) && ( ! changed ) && ( adapter->LastResult != gui->Adapter.LastResult )
			 && ( adapter->LastResult != BT_RFCOMM_CANCELLED ) )
		{
			if ( ( ! adapter->Ready ) && ( adapter->LastResult != BT_HCI_OK ) )
			{
				Prefs_SetStatus ( gui, "Bluetooth address unavailable. You can still try Scan." );
			}
			else
			{
				Prefs_ResultStatus ( gui, adapter->LastResult, NULL );
			}
		}

		gui->HaveAdapter  = TRUE;
		gui->Registration = adapters[index].Registration;
		gui->Adapter	  = *adapter;

		Prefs_RebuildRows ( gui, ( changed || reload ) && ! gui->Busy );
	}
}

// --

static void
TakeSelection ( struct PrefsWindow * gui )
{
	struct Node *	 node;
	struct PeerRow * peer;

	ULONG value;

	value = 0;
	peer  = NULL;

	GetAttr ( LISTBROWSER_SelectedNode, gui->Gadgets[GAD_DEVICES], &value );

	node = (APTR)value;

	if ( node )
	{
		gui->ListBrowser->GetListBrowserNodeAttrs ( node, LBNA_UserData, &peer, TAG_END );
	}

	gui->HaveSelection = peer != NULL;

	if ( peer )
	{
		memcpy ( gui->Selection, peer->Address, 6 );
	}

	Prefs_RefreshControls ( gui );
}

// --

static void
PeerAction ( struct PrefsWindow * gui, ULONG id )
{
	static const struct EasyStruct confirm = {
		.es_StructSize	 = sizeof ( struct EasyStruct ),
		.es_Title		 = "Bluetooth2 Preferences",
		.es_TextFormat	 = "Forget this device and delete its saved bond?",
		.es_GadgetFormat = "Forget|Cancel",
	};
	struct PeerRow *	peer;
	enum BT2PrefsAction action;
	enum BTHCIResult	result;

	peer = Prefs_Selected ( gui );

	if ( ( ! peer ) || ( ! gui->HaveAdapter ) || ( ! gui->Adapter.Ready ) || ( gui->Busy ) )
	{
		return;
	}

	switch ( id )
	{
		case GAD_DISCONNECT:
			action = BT2_PREFS_DISCONNECT;
			break;
		case GAD_FORGET:
			if ( ! EasyRequestArgs ( gui->Window, &confirm, NULL, NULL ) )
			{
				return;
			}

			action = BT2_PREFS_FORGET;
			break;
		case GAD_AUTO:
			action = peer->Auto ? BT2_PREFS_AUTO_OFF : BT2_PREFS_AUTO_ON;
			break;
		default:
			action = BT2_PREFS_CONNECT;
			break;
	}

	result = gui->Bluetooth->control_PeerAction ( gui->Adapter.Address, peer->Address, action );

	if ( ( action == BT2_PREFS_AUTO_ON ) && ( result == BT_HCI_CONTROLLER_BUSY ) )
	{
		Prefs_SetStatus ( gui, "This stick already has file sharing set to start at boot." );
	}
	else
	{
		Prefs_ResultStatus ( gui, result, NULL );
	}

	RefreshAdapter ( gui, FALSE );

	Prefs_RebuildRows ( gui, result == BT_HCI_OK );
}

// --

static void
WindowEvents ( struct PrefsWindow * gui )
{
	ULONG event;
	ULONG id;
	ULONG window;

	while ( ! gui->Closing && ( ( event = IDoMethod ( gui->Object, WM_HANDLEINPUT, NULL ) ) != WMHI_LASTMSG ) )
	{
		switch ( event & WMHI_CLASSMASK )
		{
			case WMHI_CLOSEWINDOW:
				gui->Closing = TRUE;
				break;

			case WMHI_ICONIFY:
				IDoMethod ( gui->Object, WM_ICONIFY );

				window = (ULONG)gui->Window;

				GetAttr ( WINDOW_Window, gui->Object, &window );

				gui->Window = (APTR)window;

				if ( ! gui->Window )
				{
					return;
				}
				break;

			case WMHI_UNICONIFY:
				gui->Window = (APTR)IDoMethod ( gui->Object, WM_OPEN );

				if ( gui->Window )
				{
					Prefs_ShareTick ( gui );

					Prefs_RefreshControls ( gui );

					RefreshAdapter ( gui, FALSE );

					Prefs_RebuildRows ( gui, TRUE );

					Prefs_SetStatus ( gui, gui->Status );
				}
				break;

			case WMHI_GADGETUP:
				if ( ! gui->Window )
				{
					break;
				}

				id = event & WMHI_GADGETMASK;

				switch ( id )
				{
					case GAD_SHARE_DIR:
					case GAD_SHARE_RULE:
					case GAD_SHARE_BOOT:
					case GAD_SHARE_START:
						Prefs_ShareAction ( gui, id );
						break;
					case GAD_AUDIO_OUTPUT:
					case GAD_AUDIO_PLAY:
					case GAD_AUDIO_STOP:
						Prefs_AudioAction ( gui, id );
						break;
					case GAD_ADAPTER:
						RefreshAdapter ( gui, TRUE );
						break;
					case GAD_DEVICES:
						TakeSelection ( gui );
						break;
					case GAD_SCAN:
						Prefs_StartJob ( gui, FALSE );
						break;
					case GAD_PAIR:
						Prefs_StartJob ( gui, TRUE );
						break;
					case GAD_CONNECT:
					case GAD_DISCONNECT:
					case GAD_FORGET:
					case GAD_AUTO:
						PeerAction ( gui, id );
						break;
				}
				break;
		}
	}
}

// --

static void
RunWindow ( struct PrefsWindow * gui, struct TimeRequest * timer )
{
	struct MsgPort * timerPort;

	ULONG windowMask;
	ULONG timerMask;
	ULONG signals;

	windowMask = 0;
	timerPort  = timer->Request.io_Message.mn_ReplyPort;
	timerMask  = 1UL << timerPort->mp_SigBit;

	GetAttr ( WINDOW_SigMask, gui->Object, &windowMask );

	windowMask |= 1UL << gui->AppPort->mp_SigBit;

	Prefs_SetStatus ( gui, "No Bluetooth controller registered. Check the USB Bluetooth driver." );

	Prefs_ShareTick ( gui );

	Prefs_RefreshControls ( gui );

	RefreshAdapter ( gui, FALSE );

	timer->Request.io_Command = TR_ADDREQUEST;
	timer->Time.Seconds		  = 1;
	timer->Time.Microseconds  = 0;

	SendIO ( (APTR)timer );

	while ( ! gui->Closing )
	{
		signals = Wait ( windowMask | timerMask | SIGBREAKF_CTRL_C );

		if ( signals & SIGBREAKF_CTRL_C )
		{
			gui->Closing = TRUE;
		}

		if ( ( signals & windowMask ) && ( ! gui->Closing ) )
		{
			WindowEvents ( gui );

			GetAttr ( WINDOW_SigMask, gui->Object, &windowMask );

			windowMask |= 1UL << gui->AppPort->mp_SigBit;
		}

		if ( gui->Closing )
		{
			windowMask = 0;
		}

		if ( ( signals & timerMask ) && ( GetMsg ( timerPort ) ) )
		{
			if ( gui->Closing )
			{
				continue;
			}

			Prefs_ShareTick ( gui );

			Prefs_PollJob ( gui );

			Prefs_AudioTick ( gui );

			if ( ( gui->Window ) && ( ! gui->Busy ) )
			{
				RefreshAdapter ( gui, FALSE );
			}

			timer->Time.Seconds		 = 1;
			timer->Time.Microseconds = 0;

			SendIO ( (APTR)timer );
		}
	}

	if ( ! CheckIO ( (APTR)timer ) )
	{
		AbortIO ( (APTR)timer );
	}

	WaitIO ( (APTR)timer );
}

// --

int
main ( void )
{
	struct PrefsWindow * gui;
	struct MsgPort *	 port;
	struct IORequest *	 request;
	struct TimeRequest * timer;

	int result;

	result	= RETURN_FAIL;
	port	= NULL;
	request = NULL;
	timer	= NULL;

	gui = AllocVecTags ( sizeof ( *gui ), AVT_ClearWithValue, 0, TAG_END );

	if ( ! gui )
	{
		goto done;
	}

	port = AllocSysObjectTags ( ASOT_PORT, TAG_END );

	if ( ! port )
	{
		goto done;
	}

	request = AllocSysObjectTags ( ASOT_IOREQUEST, ASOIOR_ReplyPort, port, TAG_END );

	timer = AllocSysObjectTags ( ASOT_IOREQUEST, ASOIOR_ReplyPort, port, ASOIOR_Size, sizeof ( *timer ), TAG_END );

	if ( ( ! request ) || ( ! timer ) )
	{
		goto done;
	}

	if ( OpenDevice ( BLUETOOTH2_NAME, 0, request, 0 ) )
	{
		PutStr ( "Bluetooth2 Preferences requires bluetooth2.device.\n" );

		goto done;
	}

	gui->Bluetooth = (APTR)GetInterface ( (APTR)request->io_Device, BLUETOOTH2_INTERFACE, BLUETOOTH2_INTERFACE_VERSION, NULL );

	if ( ! gui->Bluetooth )
	{
		PutStr ( "Bluetooth2 Preferences requires bluetooth2.device main interface version 4.\n" );

		goto close_device;
	}

	if ( GetVar ( "Bluetooth2/shared-folder", gui->SharePath, sizeof ( gui->SharePath ), GVF_GLOBAL_ONLY )
		 >= (LONG)sizeof ( gui->SharePath ) - 1 )
	{
		gui->SharePath[0] = 0;
	}

	gui->Bluetooth->control_StartupShare ( &gui->BootShare, FALSE );

	gui->Bluetooth->control_AudioOutput ( &gui->AudioOutput, FALSE );

	gui->AudioJob = gui->Bluetooth->control_Playback ();

	if ( ! OpenDevice ( TIMERNAME, UNIT_VBLANK, (APTR)timer, 0 ) )
	{
		if ( Prefs_OpenWindow ( gui ) )
		{
			RunWindow ( gui, timer );

			result = RETURN_OK;
		}

		Prefs_CloseWindow ( gui );

		CloseDevice ( (APTR)timer );
	}

	DropInterface ( (APTR)gui->Bluetooth );

close_device:
	CloseDevice ( request );

done:
	if ( timer )
	{
		FreeSysObject ( ASOT_IOREQUEST, timer );
	}

	if ( request )
	{
		FreeSysObject ( ASOT_IOREQUEST, request );
	}

	if ( port )
	{
		FreeSysObject ( ASOT_PORT, port );
	}

	if ( gui )
	{
		FreeVec ( gui );
	}

	return ( result );
}

// --
