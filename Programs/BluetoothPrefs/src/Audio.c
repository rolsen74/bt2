
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <stdio.h>
#include <string.h>
#include <libraries/asl.h>
#include <interfaces/asl.h>
#include "Prefs.h"

// --

void
Prefs_RefreshAudio ( struct PrefsWindow * gui )
{
	struct PeerRow * peer;

	char local[18];
	char remote[18];

	if ( ! gui->Window )
	{
		return;
	}

	peer = Prefs_Selected ( gui );

	RefreshSetGadgetAttrs ( (struct Gadget *)gui->Gadgets[GAD_AUDIO_OUTPUT], gui->Window, NULL, GA_Disabled,
							gui->AudioJob || ! ( peer && peer->Saved && gui->HaveAdapter && gui->Adapter.Ready ), TAG_END );

	RefreshSetGadgetAttrs ( (struct Gadget *)gui->Gadgets[GAD_AUDIO_PLAY], gui->Window, NULL, GA_Disabled,
							gui->AudioJob || ! gui->AudioOutput.Selected, TAG_END );

	RefreshSetGadgetAttrs ( (struct Gadget *)gui->Gadgets[GAD_AUDIO_STOP], gui->Window, NULL, GA_Disabled, ! gui->AudioJob,
							TAG_END );

	RefreshSetGadgetAttrs ( (struct Gadget *)gui->Gadgets[GAD_AUDIO_STATUS], gui->Window, NULL, GA_Text, " ", TAG_END );

	if ( gui->AudioOutput.Selected )
	{
		Prefs_FormatAddress ( local, gui->AudioOutput.Local );

		Prefs_FormatAddress ( remote, gui->AudioOutput.Peer );

		snprintf ( gui->AudioLabel, sizeof ( gui->AudioLabel ), "%s%s via %s", gui->AudioJob ? "WAV requested for " : "", remote,
				   local );
	}
	else
	{
		snprintf ( gui->AudioLabel, sizeof ( gui->AudioLabel ), "No Bluetooth audio output selected" );
	}

	RefreshSetGadgetAttrs ( (struct Gadget *)gui->Gadgets[GAD_AUDIO_STATUS], gui->Window, NULL, GA_Text, gui->AudioLabel,
							TAG_END );
}

// --

static void
SaveAudioOutput ( struct PrefsWindow * gui )
{
	struct BT2AudioOutput settings;
	struct PeerRow *	  peer;
	enum BTHCIResult	  result;

	peer = Prefs_Selected ( gui );

	if ( ( ! peer ) || ( ! peer->Saved ) || ( ! gui->HaveAdapter ) || ( ! gui->Adapter.Ready ) || ( gui->AudioJob ) )
	{
		return;
	}

	settings.Selected = TRUE;

	memcpy ( settings.Local, gui->Adapter.Address, sizeof ( settings.Local ) );

	memcpy ( settings.Peer, peer->Address, sizeof ( settings.Peer ) );

	result = gui->Bluetooth->control_AudioOutput ( &settings, TRUE );

	if ( result == BT_HCI_OK )
	{
		gui->AudioOutput = settings;

		Prefs_SetStatus ( gui, "Bluetooth audio output saved." );

		Prefs_RefreshAudio ( gui );
	}
	else
	{
		Prefs_ResultStatus ( gui, result, NULL );
	}
}

// --

static void
PlayWAV ( struct PrefsWindow * gui )
{
	struct Library *	   library;
	struct AslIFace *	   asl;
	struct FileRequester * request;
	enum BTHCIResult	   result;

	char path[BT_SHARE_PATH_SIZE];

	if ( ( ! gui->AudioOutput.Selected ) || ( gui->AudioJob ) )
	{
		return;
	}

	library = OpenLibrary ( "asl.library", 50 );

	if ( ! library )
	{
		Prefs_SetStatus ( gui, "Could not open the file requester." );

		return;
	}

	asl = (APTR)GetInterface ( library, "main", 1, NULL );

	if ( asl )
	{
		request = asl->AllocAslRequestTags ( ASL_FileRequest, ASLFR_Window, gui->Window, ASLFR_TitleText,
											 "Play a WAV file over Bluetooth", TAG_END );

		if ( request )
		{
			if ( asl->AslRequestTags ( request, TAG_END ) )
			{
				result = BT_HCI_INVALID_ARGUMENT;

				if ( strlen ( request->fr_Drawer ) < sizeof ( path ) )
				{
					strcpy ( path, request->fr_Drawer );

					if ( AddPart ( path, request->fr_File, sizeof ( path ) ) )
					{
						result = gui->Bluetooth->control_PlayWAV ( path, &gui->AudioJob );
					}
				}

				if ( result == BT_HCI_OK )
				{
					Prefs_SetStatus ( gui, "Starting Bluetooth WAV playback..." );
				}
				else
				{
					Prefs_ResultStatus ( gui, result, NULL );
				}
			}

			asl->FreeAslRequest ( request );
		}

		DropInterface ( (APTR)asl );
	}

	CloseLibrary ( library );

	Prefs_RefreshAudio ( gui );
}

// --

void
Prefs_AudioAction ( struct PrefsWindow * gui, ULONG id )
{
	enum BTHCIResult result;

	switch ( id )
	{
		case GAD_AUDIO_OUTPUT:
			SaveAudioOutput ( gui );
			break;

		case GAD_AUDIO_PLAY:
			PlayWAV ( gui );
			break;

		case GAD_AUDIO_STOP:
			if ( gui->AudioJob )
			{
				result = gui->Bluetooth->control_Cancel ( gui->AudioJob );

				if ( result == BT_HCI_OK )
				{
					Prefs_SetStatus ( gui, "Stopping WAV playback..." );
				}
			}
			break;
	}
}

// --

void
Prefs_AudioTick ( struct PrefsWindow * gui )
{
	struct BT2OperationStatus status;
	enum BTHCIResult		  result;

	if ( ! gui->AudioJob )
	{
		return;
	}

	memset ( &status, 0, sizeof ( status ) );

	result = gui->Bluetooth->control_Operation ( gui->AudioJob, &status, NULL, 0 );

	if ( ( result == BT_HCI_OK ) && ( status.Pending ) )
	{
		return;
	}

	gui->AudioJob = 0;

	if ( ( result == BT_HCI_OK ) && ( status.Result == BT_HCI_OK || status.Result == BT_RFCOMM_CANCELLED ) )
	{
		Prefs_SetStatus ( gui, "Bluetooth WAV playback ended." );
	}
	else
	{
		Prefs_ResultStatus ( gui, result == BT_HCI_OK ? status.Result : result, &status.Error );
	}

	Prefs_RefreshAudio ( gui );
}

// --
