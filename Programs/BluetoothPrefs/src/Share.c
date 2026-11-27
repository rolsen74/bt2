
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <dos/var.h>
#include <libraries/asl.h>
#include <interfaces/asl.h>
#include <stdio.h>
#include <string.h>
#include "Prefs.h"

// --

#define SHARE_FOLDER_VAR "Bluetooth2/shared-folder"

void
Prefs_RefreshShare ( struct PrefsWindow * gui )
{
	struct PeerRow * peer;

	LONG		 access;
	int			 canEdit;
	int			 waiting;
	const char * boot;
	char		 address[18];

	if ( ! gui->Window )
	{
		return;
	}

	peer = Prefs_Selected ( gui );

	canEdit = ! gui->Busy && gui->HaveAdapter && gui->Adapter.Ready && peer && peer->Saved;
	access	= canEdit ? gui->Bluetooth->control_ShareRule ( gui->Adapter.Address, peer->Address, BT_SHARE_RULE_GET ) : -1;

	RefreshSetGadgetAttrs ( (struct Gadget *)gui->Gadgets[GAD_SHARE_RULE], gui->Window, NULL, GA_Disabled, access < 0,
							CHOOSER_Selected, access < 0 ? 0 : access, TAG_END );

	RefreshSetGadgetAttrs ( (struct Gadget *)gui->Gadgets[GAD_SHARE_DIR], gui->Window, NULL, GA_Disabled, gui->Sharing,
							GA_HintInfo, gui->SharePath[0] ? gui->SharePath : "Select the shared folder", TAG_END );

	waiting = gui->BootShareState == BT2_BOOT_SHARE_WAITING;

	RefreshSetGadgetAttrs ( (struct Gadget *)gui->Gadgets[GAD_SHARE_START], gui->Window, NULL, GA_Disabled,
							( gui->ShareStatus.State == BT_SHARE_STOPPING )
								|| ( ! gui->Sharing && ! waiting
									 && ( gui->Busy || ! gui->HaveAdapter || ! gui->Adapter.Ready || gui->Adapter.Busy
										  || gui->Adapter.State != BT2_PREFS_IDLE || ! gui->SharePath[0] ) ),
							GA_Text, ( gui->Sharing || waiting ) ? "Stop sharing" : "Start sharing", TAG_END );

	RefreshSetGadgetAttrs ( (struct Gadget *)gui->Gadgets[GAD_SHARE_BOOT], gui->Window, NULL, GA_Text,
							gui->BootShare.Enabled ? "Disable at boot" : "Enable at boot", GA_Disabled,
							! gui->BootShare.Enabled && ( ! gui->HaveAdapter || ! gui->Adapter.Ready || ! gui->SharePath[0] ),
							TAG_END );

	RefreshSetGadgetAttrs ( (struct Gadget *)gui->Gadgets[GAD_SHARE_STATUS], gui->Window, NULL, GA_Text, " ", TAG_END );

	if ( gui->Sharing )
	{
		Prefs_FormatAddress ( address, gui->ShareStatus.Local );

		snprintf ( gui->ShareLabel, sizeof ( gui->ShareLabel ), "%s on %s%s",
				   gui->ShareStatus.State == BT_SHARE_LISTENING	 ? "Sharing"
				   : gui->ShareStatus.State == BT_SHARE_STOPPING ? "Stopping share"
																 : "Starting share",
				   address, gui->BootShare.Enabled ? "  (startup enabled)" : "" );
	}
	else if ( gui->BootShare.Enabled )
	{
		Prefs_FormatAddress ( address, gui->BootShare.Local );

		switch ( gui->BootShareState )
		{
			case BT2_BOOT_SHARE_WAITING:
				boot = "Startup share waiting";
				break;
			case BT2_BOOT_SHARE_STOPPED:
				boot = "Startup share stopped";
				break;
			case BT2_BOOT_SHARE_CONFLICT:
				boot = "Startup conflict";
				break;
			case BT2_BOOT_SHARE_FOLDER_ERROR:
				boot = "Startup folder unavailable";
				break;
			default:
				boot = "Share at next boot";
				break;
		}

		snprintf ( gui->ShareLabel, sizeof ( gui->ShareLabel ), "%s on %s", boot, address );
	}
	else
	{
		snprintf ( gui->ShareLabel, sizeof ( gui->ShareLabel ), "%.95s",
				   gui->SharePath[0] ? gui->SharePath : "No shared folder" );
	}

	RefreshSetGadgetAttrs ( (struct Gadget *)gui->Gadgets[GAD_SHARE_STATUS], gui->Window, NULL, GA_Text, gui->ShareLabel,
							TAG_END );
}

// --

static void
ShowFolder ( struct PrefsWindow * gui )
{
	char status[sizeof ( gui->Status )];

	snprintf ( status, sizeof ( status ), "%s %.74s", gui->Sharing ? "Sharing" : "Shared folder",
			   gui->Sharing ? gui->ShareStatus.Path : gui->SharePath );

	Prefs_SetStatus ( gui, status );
}

// --

static void
EnsurePrefsDirectory ( const char * path )
{
	BPTR lock;

	lock = CreateDirTree ( path );

	if ( lock )
	{
		UnLock ( lock );
	}
}

// --

static void
ChooseFolder ( struct PrefsWindow * gui )
{
	struct Library *	   library;
	struct AslIFace *	   asl;
	struct FileRequester * request;

	char path[BT_SHARE_PATH_SIZE];
	BPTR lock;

	library = OpenLibrary ( "asl.library", 50 );

	if ( ! library )
	{
		Prefs_SetStatus ( gui, "Could not open the folder requester." );

		return;
	}

	asl = (APTR)GetInterface ( library, "main", 1, NULL );

	if ( asl )
	{
		request = asl->AllocAslRequestTags ( ASL_FileRequest, ASLFR_Window, gui->Window, ASLFR_TitleText,
											 "Select Bluetooth shared folder", ASLFR_DrawersOnly, TRUE, ASLFR_InitialDrawer,
											 gui->SharePath, TAG_END );

		if ( request )
		{
			if ( asl->AslRequestTags ( request, TAG_END ) )
			{
				lock = Lock ( request->fr_Drawer, SHARED_LOCK );

				if ( ( lock ) && ( NameFromLock ( lock, path, sizeof ( path ) ) ) )
				{
					EnsurePrefsDirectory ( "ENV:Bluetooth2" );

					EnsurePrefsDirectory ( "ENVARC:Bluetooth2" );

					if ( SetVar ( SHARE_FOLDER_VAR, path, strlen ( path ), GVF_GLOBAL_ONLY | GVF_SAVE_VAR ) )
					{
						strcpy ( gui->SharePath, path );

						ShowFolder ( gui );
					}
					else
					{
						Prefs_SetStatus ( gui, "Could not save the shared folder." );
					}
				}
				else
				{
					Prefs_SetStatus ( gui, "Could not open the selected folder." );
				}

				if ( lock )
				{
					UnLock ( lock );
				}
			}

			asl->FreeAslRequest ( request );
		}

		DropInterface ( (APTR)asl );
	}

	CloseLibrary ( library );

	Prefs_RefreshShare ( gui );
}

// --

static void
StartShare ( struct PrefsWindow * gui )
{
	enum BTHCIResult result;

	if ( ( gui->Busy ) || ( ! gui->HaveAdapter ) || ( ! gui->Adapter.Ready ) || ( ! gui->SharePath[0] ) )
	{
		return;
	}

	result = gui->Bluetooth->control_StartShare ( gui->Adapter.Address, gui->SharePath );

	if ( result != BT_HCI_OK )
	{
		Prefs_ResultStatus ( gui, result, NULL );
	}
	else
	{
		Prefs_ShareTick ( gui );
	}
}

// --

static void
ToggleBootShare ( struct PrefsWindow * gui )
{
	struct BT2StartupShare settings;
	enum BTHCIResult	   result;

	settings		 = gui->BootShare;
	settings.Enabled = ! settings.Enabled;

	if ( settings.Enabled )
	{
		if ( ( ! gui->HaveAdapter ) || ( ! gui->Adapter.Ready ) || ( ! gui->SharePath[0] ) )
		{
			return;
		}

		memcpy ( settings.Local, gui->Adapter.Address, sizeof ( settings.Local ) );

		strcpy ( settings.Path, gui->SharePath );
	}

	result = gui->Bluetooth->control_StartupShare ( &settings, TRUE );

	if ( result == BT_HCI_OK )
	{
		gui->BootShare = settings;

		Prefs_SetStatus ( gui, settings.Enabled ? "Saved file sharing for the next boot." : "Startup file sharing disabled." );

		Prefs_RefreshShare ( gui );
	}
	else if ( result == BT_HCI_CONTROLLER_BUSY )
	{
		Prefs_SetStatus ( gui, "This stick already has input set to connect at boot." );
	}
	else
	{
		Prefs_ResultStatus ( gui, result, NULL );
	}
}

// --

void
Prefs_ShareAction ( struct PrefsWindow * gui, ULONG id )
{
	struct PeerRow * peer;

	ULONG access;

	switch ( id )
	{
		case GAD_SHARE_DIR:
			if ( ! gui->Sharing )
			{
				ChooseFolder ( gui );
			}
			break;

		case GAD_SHARE_RULE:
			peer = Prefs_Selected ( gui );

			if ( ( peer ) && ( peer->Saved ) && ( gui->HaveAdapter ) && ( gui->Adapter.Ready ) && ( ! gui->Busy ) )
			{
				access = 0;

				GetAttr ( CHOOSER_Selected, gui->Gadgets[GAD_SHARE_RULE], &access );

				if ( gui->Bluetooth->control_ShareRule ( gui->Adapter.Address, peer->Address, access ) < 0 )
				{
					Prefs_SetStatus ( gui, "Could not save the selected device's file access." );

					Prefs_RefreshShare ( gui );
				}
			}
			break;

		case GAD_SHARE_BOOT:
			ToggleBootShare ( gui );
			break;

		case GAD_SHARE_START:
			if ( ( gui->Sharing ) || ( gui->BootShareState == BT2_BOOT_SHARE_WAITING ) )
			{
				if ( gui->Bluetooth->control_StopShare () == BT_HCI_OK )
				{
					Prefs_ShareTick ( gui );
				}
			}
			else
			{
				StartShare ( gui );
			}
			break;
	}
}

// --

void
Prefs_ShareTick ( struct PrefsWindow * gui )
{
	struct BTShareStatus	  previous;
	enum BT2StartupShareState lastBoot;

	previous = gui->ShareStatus;
	lastBoot = gui->BootShareState;

	gui->BootShareState = gui->Bluetooth->control_ShareStartupState ();

	gui->Bluetooth->sharecontrol_Status ( &gui->ShareStatus );

	if ( ( previous.State == gui->ShareStatus.State ) && ( previous.Result == gui->ShareStatus.Result )
		 && ( previous.Error.TransportError == gui->ShareStatus.Error.TransportError )
		 && ( previous.Error.Opcode == gui->ShareStatus.Error.Opcode )
		 && ( previous.Error.Status == gui->ShareStatus.Error.Status )
		 && ( ! memcmp ( previous.Local, gui->ShareStatus.Local, sizeof ( previous.Local ) ) )
		 && ( ! strcmp ( previous.Path, gui->ShareStatus.Path ) ) )
	{
		if ( ( gui->Window ) && ( lastBoot != gui->BootShareState ) )
		{
			Prefs_RefreshShare ( gui );
		}

		return;
	}

	gui->Sharing = gui->ShareStatus.State != BT_SHARE_STOPPED;

	switch ( gui->ShareStatus.State )
	{
		case BT_SHARE_STARTING:
			Prefs_SetStatus ( gui, "Starting Bluetooth file sharing..." );
			break;

		case BT_SHARE_LISTENING:
			ShowFolder ( gui );
			break;

		case BT_SHARE_STOPPING:
			Prefs_SetStatus ( gui, "Stopping Bluetooth file sharing..." );
			break;

		case BT_SHARE_STOPPED:
			if ( ( gui->ShareStatus.Result == BT_RFCOMM_CANCELLED ) || ( gui->ShareStatus.Result == BT_HCI_OK ) )
			{
				Prefs_SetStatus ( gui, "Bluetooth file sharing stopped." );
			}
			else
			{
				Prefs_ResultStatus ( gui, gui->ShareStatus.Result, &gui->ShareStatus.Error );
			}
			break;
	}

	if ( gui->Window )
	{
		Prefs_RefreshControls ( gui );
	}
}

// --
