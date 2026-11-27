
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <stdio.h>
#include <string.h>
#include <images/bevel.h>
#include <dos/var.h>
#include <gadgets/button.h>
#include "Prefs.h"

// --

struct IntuitionIFace * IIntuition;

static const char * const Classes[] = {
	"window.class",			 "gadgets/layout.gadget", "gadgets/listbrowser.gadget",
	"gadgets/string.gadget", "gadgets/button.gadget", "gadgets/chooser.gadget",
};

static const char * const ShareRules[] = { "No access", "Read only", "Write only", "Read and write", NULL };

static const struct TagItem FixedHeight[] = { { CHILD_WeightedHeight, 0 }, { TAG_END, 0 } };

// --

static Object *
AddChild ( Object * layout, Object * child, const struct TagItem * tags )
{
	if ( ( child ) && ( ! IDoMethod ( layout, LM_ADDCHILD, NULL, child, tags ) ) )
	{
		DisposeObject ( child );

		child = NULL;
	}

	return ( child );
}

// --

static Object *
MakeGroup ( struct PrefsWindow * gui, Object * layout )
{
	Object * group;

	group = NewObject ( gui->Classes[CLASS_LAYOUT], NULL, LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ, LAYOUT_SpaceInner, TRUE,
						LAYOUT_SpaceOuter, TRUE, LAYOUT_BevelStyle, BVS_THIN, TAG_END );

	return ( AddChild ( layout, group, FixedHeight ) );
}

// --

static int
AddButton ( struct PrefsWindow * gui, Object * row, ULONG id, const char * text )
{
	Object * button;

	button = NewObject ( gui->Classes[CLASS_BUTTON], NULL, GA_ID, id, GA_RelVerify, TRUE, GA_Text, text, GA_Underscore, '_',
						 TAG_END );

	gui->Gadgets[id] = AddChild ( row, button, NULL );

	return ( gui->Gadgets[id] != NULL );
}

// --

static int
BuildControls ( struct PrefsWindow * gui, Object * layout )
{
	Object * group;
	Object * control;

	group = MakeGroup ( gui, layout );

	if ( ! group )
	{
		return ( FALSE );
	}

	gui->AdapterLabels[0] = "No adapter";

	control = NewObject ( gui->Classes[CLASS_CHOOSER], NULL, GA_ID, GAD_ADAPTER, GA_RelVerify, TRUE, CHOOSER_PopUp, TRUE,
						  CHOOSER_LabelArray, gui->AdapterLabels, TAG_END );

	gui->Gadgets[GAD_ADAPTER] = AddChild ( group, control, NULL );

	if ( ( ! gui->Gadgets[GAD_ADAPTER] ) || ( ! AddButton ( gui, group, GAD_SCAN, "_Scan" ) ) )
	{
		return ( FALSE );
	}

	control = NewObject ( gui->Classes[CLASS_LIST], NULL, GA_ID, GAD_DEVICES, GA_RelVerify, TRUE, LISTBROWSER_ShowSelected, TRUE,
						  LISTBROWSER_Labels, &gui->Rows, LISTBROWSER_MinVisible, 7, LISTBROWSER_ColumnInfo, gui->Columns,
						  LISTBROWSER_ColumnTitles, TRUE, TAG_END );

	gui->Gadgets[GAD_DEVICES] = AddChild ( layout, control, NULL );

	group = MakeGroup ( gui, layout );

	if ( ( ! gui->Gadgets[GAD_DEVICES] ) || ( ! group ) )
	{
		return ( FALSE );
	}

	control = NewObject ( gui->Classes[CLASS_BUTTON], NULL, GA_Text, "PIN", GA_ReadOnly, TRUE, BUTTON_Transparent, TRUE,
						  BUTTON_BevelStyle, BVS_NONE, TAG_END );

	if ( ! AddChild ( group, control, NULL ) )
	{
		return ( FALSE );
	}

	control = NewObject ( gui->Classes[CLASS_STRING], NULL, GA_ID, GAD_PIN, GA_TabCycle, TRUE, STRINGA_MaxChars, BT_PIN_MAX + 1,
						  STRINGA_MinVisible, 12, STRINGA_HookType, SHK_PASSWORD, STRINGA_Buffer, gui->PIN, STRINGA_UndoBuffer,
						  gui->PINUndo, STRINGA_WorkBuffer, gui->PINWork, STRINGA_DisablePopup, TRUE, STRINGA_AllowMarking,
						  FALSE, TAG_END );

	gui->Gadgets[GAD_PIN] = AddChild ( group, control, NULL );

	if ( ( ! gui->Gadgets[GAD_PIN] ) || ( ! AddButton ( gui, group, GAD_PAIR, "_Pair" ) ) )
	{
		return ( FALSE );
	}

	group = MakeGroup ( gui, layout );

	if ( ( ! group ) || ( ! AddButton ( gui, group, GAD_CONNECT, "_Connect" ) )
		 || ( ! AddButton ( gui, group, GAD_DISCONNECT, "_Disconnect" ) )
		 || ( ! AddButton ( gui, group, GAD_FORGET, "_Forget" ) ) || ( ! AddButton ( gui, group, GAD_AUTO, "_Auto-connect" ) ) )
	{
		return ( FALSE );
	}

	group = NewObject ( gui->Classes[CLASS_LAYOUT], NULL, LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ, LAYOUT_SpaceInner, TRUE,
						LAYOUT_SpaceOuter, TRUE, LAYOUT_BevelStyle, BVS_THIN, LAYOUT_Label, "File sharing", LAYOUT_LabelPlace,
						BVJ_TOP_LEFT, TAG_END );

	group = AddChild ( layout, group, FixedHeight );

	if ( ( ! group ) || ( ! AddButton ( gui, group, GAD_SHARE_DIR, "Shared _folder..." ) ) )
	{
		return ( FALSE );
	}

	control = NewObject ( gui->Classes[CLASS_CHOOSER], NULL, GA_ID, GAD_SHARE_RULE, GA_RelVerify, TRUE, CHOOSER_PopUp, TRUE,
						  CHOOSER_LabelArray, ShareRules, GA_HintInfo, "File access for the selected paired device", TAG_END );

	gui->Gadgets[GAD_SHARE_RULE] = AddChild ( group, control, NULL );

	if ( ( ! gui->Gadgets[GAD_SHARE_RULE] ) || ( ! AddButton ( gui, group, GAD_SHARE_START, "Start sharing" ) )
		 || ( ! AddButton ( gui, group, GAD_SHARE_BOOT, "Enable at boot" ) ) )
	{
		return ( FALSE );
	}

	control = NewObject ( gui->Classes[CLASS_BUTTON], NULL, GA_Text, " ", GA_ReadOnly, TRUE, BUTTON_Justification, BCJ_LEFT,
						  BUTTON_Transparent, TRUE, BUTTON_BevelStyle, BVS_NONE, TAG_END );

	gui->Gadgets[GAD_SHARE_STATUS] = AddChild ( layout, control, FixedHeight );

	if ( ! gui->Gadgets[GAD_SHARE_STATUS] )
	{
		return ( FALSE );
	}

	group = NewObject ( gui->Classes[CLASS_LAYOUT], NULL, LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ, LAYOUT_SpaceInner, TRUE,
						LAYOUT_SpaceOuter, TRUE, LAYOUT_BevelStyle, BVS_THIN, LAYOUT_Label, "Audio output", LAYOUT_LabelPlace,
						BVJ_TOP_LEFT, TAG_END );

	group = AddChild ( layout, group, FixedHeight );

	if ( ( ! group ) || ( ! AddButton ( gui, group, GAD_AUDIO_OUTPUT, "Use selected device" ) )
		 || ( ! AddButton ( gui, group, GAD_AUDIO_PLAY, "Play WAV..." ) )
		 || ( ! AddButton ( gui, group, GAD_AUDIO_STOP, "Stop" ) ) )
	{
		return ( FALSE );
	}

	control = NewObject ( gui->Classes[CLASS_BUTTON], NULL, GA_Text, " ", GA_ReadOnly, TRUE, BUTTON_Justification, BCJ_LEFT,
						  BUTTON_Transparent, TRUE, BUTTON_BevelStyle, BVS_NONE, TAG_END );

	gui->Gadgets[GAD_AUDIO_STATUS] = AddChild ( layout, control, FixedHeight );

	if ( ! gui->Gadgets[GAD_AUDIO_STATUS] )
	{
		return ( FALSE );
	}

	control = NewObject ( gui->Classes[CLASS_BUTTON], NULL, GA_Text, " ", GA_ReadOnly, TRUE, BUTTON_Justification, BCJ_LEFT,
						  BUTTON_Transparent, TRUE, BUTTON_BevelStyle, BVS_NONE, TAG_END );

	gui->Gadgets[GAD_STATUS] = AddChild ( layout, control, FixedHeight );

	return ( gui->Gadgets[GAD_STATUS] != NULL );
}

// --

void
Prefs_SetStatus ( struct PrefsWindow * gui, const char * text )
{
	char previous[sizeof ( gui->Status )];

	// Change the text pointer as well as its contents to force a refresh.
	snprintf ( previous, sizeof ( previous ), "%s", text );

	if ( gui->Window )
	{
		RefreshSetGadgetAttrs ( (struct Gadget *)gui->Gadgets[GAD_STATUS], gui->Window, NULL, GA_Text, " ", TAG_END );
	}

	memcpy ( gui->Status, previous, strlen ( previous ) + 1 );

	if ( gui->Window )
	{
		RefreshSetGadgetAttrs ( (struct Gadget *)gui->Gadgets[GAD_STATUS], gui->Window, NULL, GA_Text, gui->Status, TAG_END );
	}
}

// --

void
Prefs_ResultStatus ( struct PrefsWindow * gui, enum BTHCIResult result, const struct BTHCIError * error )
{
	char		 text[96];
	const char * message;

	switch ( result )
	{
		case BT_HCI_OK:
			message = "Ready.";
			break;
		case BT_HCI_NO_CONTROLLER:
			message = "Bluetooth adapter unavailable.";
			break;
		case BT_HCI_CONTROLLER_BUSY:
			message = "Bluetooth adapter is busy.";
			break;
		case BT_HCI_NO_SAVED_KEY:
			message = "Pair this device before connecting.";
			break;
		case BT_HCI_BOND_IO_ERROR:
			message = "Could not update Bluetooth preferences or the saved bond.";
			break;
		case BT_HCI_UNSUPPORTED:
			message = "This operation is unavailable on the selected controller.";
			break;
		case BT_ACL_READ_ERROR:
		case BT_ACL_WRITE_ERROR:
			if ( error )
			{
				snprintf ( text, sizeof ( text ), "Bluetooth data %s failed (transport %ld).",
						   result == BT_ACL_READ_ERROR ? "read" : "write", (long)error->TransportError );
			}
			else
			{
				snprintf ( text, sizeof ( text ), "Bluetooth data %s failed. See the serial log.",
						   result == BT_ACL_READ_ERROR ? "read" : "write" );
			}

			message = text;
			break;
		case BT_HCI_READ_ERROR:
		case BT_HCI_WRITE_ERROR:
		case BT_HCI_COMMAND_ERROR:
			snprintf ( text, sizeof ( text ), "HCI %s failed ($%04X, status $%02X, transport %ld).",
					   result == BT_HCI_READ_ERROR	  ? "event read"
					   : result == BT_HCI_WRITE_ERROR ? "command write"
													  : "command",
					   error ? (unsigned)error->Opcode : 0U, error ? (unsigned)error->Status : 0U,
					   error ? (long)error->TransportError : 0L );

			message = text;
			break;
		default:
			if ( error )
			{
				snprintf ( text, sizeof ( text ), "Bluetooth operation failed (%u, transport %ld).", (unsigned)result,
						   (long)error->TransportError );
			}
			else
			{
				snprintf ( text, sizeof ( text ), "Bluetooth operation failed (%u). See the serial log.", (unsigned)result );
			}

			message = text;
			break;
	}

	Prefs_SetStatus ( gui, message );
}

// --

int
Prefs_OpenWindow ( struct PrefsWindow * gui )
{
	struct Library * intuition;
	Object *		 layout;

	unsigned index;

	intuition = OpenLibrary ( "intuition.library", 50 );

	if ( ! intuition )
	{
		return ( FALSE );
	}

	IIntuition = (APTR)GetInterface ( intuition, "main", 1, NULL );

	if ( ! IIntuition )
	{
		CloseLibrary ( intuition );

		return ( FALSE );
	}

	NewList ( &gui->Rows );

	for ( index = 0; index < CLASS_COUNT; index++ )
	{
		gui->ClassLibraries[index] = OpenClass ( Classes[index], 50, &gui->Classes[index] );

		if ( ! gui->ClassLibraries[index] )
		{
			return ( FALSE );
		}
	}

	gui->ListBrowser = (APTR)GetInterface ( (APTR)gui->ClassLibraries[CLASS_LIST], "main", 1, NULL );

	if ( ! gui->ListBrowser )
	{
		return ( FALSE );
	}

	gui->Columns = gui->ListBrowser->AllocLBColumnInfo ( 4, LBCIA_Column, 0, LBCIA_Title, "Address", LBCIA_Weight, 32,
														 LBCIA_Column, 1, LBCIA_Title, "Type", LBCIA_Weight, 26, LBCIA_Column, 2,
														 LBCIA_Title, "Status", LBCIA_Weight, 22, LBCIA_Column, 3, LBCIA_Title,
														 "Auto-connect", LBCIA_Weight, 20, TAG_END );

	if ( ! gui->Columns )
	{
		return ( FALSE );
	}

	layout = NewObject ( gui->Classes[CLASS_LAYOUT], NULL, LAYOUT_Orientation, LAYOUT_ORIENT_VERT, LAYOUT_SpaceInner, TRUE,
						 LAYOUT_SpaceOuter, TRUE, TAG_END );

	if ( ! layout )
	{
		return ( FALSE );
	}

	if ( ! BuildControls ( gui, layout ) )
	{
		DisposeObject ( layout );

		return ( FALSE );
	}

	gui->AppPort = AllocSysObjectTags ( ASOT_PORT, TAG_END );

	if ( ! gui->AppPort )
	{
		DisposeObject ( layout );

		return ( FALSE );
	}

	gui->Object
		= NewObject ( gui->Classes[CLASS_WINDOW], NULL, WA_Title, "Bluetooth2 Preferences", WA_DragBar, TRUE, WA_CloseGadget,
					  TRUE, WA_DepthGadget, TRUE, WA_SizeGadget, TRUE, WA_Activate, TRUE, WA_InnerWidth, 480, WA_IDCMP,
					  IDCMP_CLOSEWINDOW | IDCMP_GADGETUP, WINDOW_Layout, layout, WINDOW_AppPort, gui->AppPort, WINDOW_IconTitle,
					  "Bluetooth2", WINDOW_IconifyGadget, TRUE, WINDOW_GadgetHelp, TRUE, TAG_END );

	if ( ! gui->Object )
	{
		DisposeObject ( layout );

		return ( FALSE );
	}

	gui->Window = (APTR)IDoMethod ( gui->Object, WM_OPEN );

	return ( gui->Window != NULL );
}

// --

void
Prefs_CloseWindow ( struct PrefsWindow * gui )
{
	struct Library * intuition;

	unsigned index;

	if ( ! IIntuition )
	{
		return;
	}

	if ( gui->Object )
	{
		DisposeObject ( gui->Object );
	}

	if ( gui->AppPort )
	{
		struct Message * message;

		while ( ( message = GetMsg ( gui->AppPort ) ) )
		{
			ReplyMsg ( message );
		}

		FreeSysObject ( ASOT_PORT, gui->AppPort );
	}

	if ( gui->ListBrowser )
	{
		gui->ListBrowser->FreeListBrowserList ( &gui->Rows );

		if ( gui->Columns )
		{
			gui->ListBrowser->FreeLBColumnInfo ( gui->Columns );
		}

		DropInterface ( (APTR)gui->ListBrowser );
	}

	for ( index = CLASS_COUNT; index; index-- )
	{
		if ( gui->ClassLibraries[index - 1] )
		{
			CloseClass ( gui->ClassLibraries[index - 1] );
		}
	}

	intuition = IIntuition->Data.LibBase;

	DropInterface ( (APTR)IIntuition );

	CloseLibrary ( intuition );
}

// --
