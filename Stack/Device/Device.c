
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_device.h"
#include "bt2_prefs.h"
#include "bt2_share.h"

// --

static const PTR ManagerVectors[] = //
	{ (PTR)_main_Obtain,
	  (PTR)_main_Release,
	  NULL,
	  NULL,
	  (PTR)_manager_Open,
	  (PTR)_manager_Close,
	  (PTR)_manager_Expunge,
	  NULL,
	  (PTR)_manager_BeginIO,
	  (PTR)_manager_AbortIO,
	  (PTR)-1 };

static const struct TagItem ManagerTags[] = //
	{
		{ MIT_Name, ( Tag ) "__device" },		  //
		{ MIT_VectorTable, (Tag)ManagerVectors }, //
		{ MIT_Version, 1 },						  //
		{ TAG_END, 0 }							  //
	};

static const PTR MainVectors[] = //
	{ (PTR)_main_Obtain,		 //
	  (PTR)_main_Release,		 //
	  NULL,						 //
	  NULL,						 //

	  (PTR)_main_controller_ResetController,
	  (PTR)_main_controller_ReadControllerInfo,
	  (PTR)_main_controller_AddController,
	  (PTR)_main_controller_RemoveController,
	  (PTR)_main_controller_ProbeController,
	  (PTR)_main_controller_DiscoverController,
	  (PTR)_main_controller_Inquiry,
	  (PTR)_main_controller_AddDiscoveryController,

	  (PTR)_main_pairing_PairController,
	  (PTR)_main_pairing_Pair,
	  (PTR)_main_pairing_AddController,

	  (PTR)_main_bonding_BondController,
	  (PTR)_main_bonding_Bond,
	  (PTR)_main_bonding_AddController,

	  (PTR)_main_names_ReadName,
	  (PTR)_main_names_ReadRemoteName,
	  (PTR)_main_names_AddController,

	  (PTR)_main_visibility_MakeVisible,
	  (PTR)_main_visibility_RunVisible,
	  (PTR)_main_visibility_AddController,

	  (PTR)_main_l2cap_EchoController,
	  (PTR)_main_l2cap_Echo,
	  (PTR)_main_l2cap_AddController,

	  (PTR)_main_sdp_BrowseController,
	  (PTR)_main_sdp_Browse,
	  (PTR)_main_sdp_AddController,

	  (PTR)_main_rfcomm_TestController,
	  (PTR)_main_rfcomm_Test,
	  (PTR)_main_rfcomm_AddController,

	  (PTR)_main_serial_ExchangeController,
	  (PTR)_main_serial_Exchange,
	  (PTR)_main_serial_AddController,

	  (PTR)_main_serialfile_SendController,
	  (PTR)_main_serialfile_Send,
	  (PTR)_main_serialfile_AddController,

	  (PTR)_main_serialcapture_ReceiveController,
	  (PTR)_main_serialcapture_Receive,
	  (PTR)_main_serialcapture_AddController,

	  (PTR)_main_objectpush_PushController,
	  (PTR)_main_objectpush_Push,
	  (PTR)_main_objectpush_AddController,

	  (PTR)_main_le_RunController,
	  (PTR)_main_le_Run,
	  (PTR)_main_le_AddController,

	  (PTR)_main_a2dp_PlayController,
	  (PTR)_main_a2dp_Play,
	  (PTR)_main_a2dp_AddController,

	  (PTR)_main_pcm_PlayController,
	  (PTR)_main_pcm_Play,
	  (PTR)_main_pcm_AddController,

	  (PTR)_main_hid_MonitorController,
	  (PTR)_main_hid_Monitor,
	  (PTR)_main_hid_AddController,

	  (PTR)_main_hidinput_ConnectController,
	  (PTR)_main_hidinput_Connect,
	  (PTR)_main_hidinput_AddController,

	  (PTR)_main_prefs_Adapters,
	  (PTR)_main_prefs_Peers,
	  (PTR)_main_prefs_Action,
	  (PTR)_main_prefs_PeerRequest,

	  (PTR)_main_fileshare_Listen,
	  (PTR)_main_fileshare_Serve,
	  (PTR)_main_fileshare_AddController,
	  (PTR)_main_fileshare_Rule,

	  (PTR)_main_sharecontrol_Start,
	  (PTR)_main_sharecontrol_Stop,
	  (PTR)_main_sharecontrol_Status,

	  (PTR)_main_control_Adapters,
	  (PTR)_main_control_Submit,
	  (PTR)_main_control_Operation,
	  (PTR)_main_control_Cancel,
	  (PTR)_main_control_StartupShare,
	  (PTR)_main_control_AudioOutput,
	  (PTR)_main_control_PlayWAV,
	  (PTR)_main_control_Playback,
	  (PTR)_main_control_PeerAction,
	  (PTR)_main_control_StartShare,
	  (PTR)_main_control_StopShare,
	  (PTR)_main_control_ShareStartupState,
	  (PTR)_main_control_ShareRule,

	  (PTR)-1 };

static const struct TagItem MainTags[] = //
	{ { MIT_Name, (Tag)BLUETOOTH2_INTERFACE },
	  { MIT_VectorTable, (Tag)MainVectors },
	  { MIT_Version, BLUETOOTH2_INTERFACE_VERSION },
	  { TAG_END, 0 } };

const PTR DeviceInterfaces[] = { (PTR)ManagerTags, (PTR)MainTags, NULL };

// --

static const char VersionTag[] __attribute__ ( ( used ) ) = "\0$VER: " BLUETOOTH2_ID;

struct Resident Bluetooth2Resident __attribute__ ( ( used ) ) = //
	{ /* rt_MatchWord	*/ RTC_MATCHWORD,
	  /* rt_MatchTag	*/ (PTR)( &Bluetooth2Resident + 0 ),
	  /* rt_EndSkip		*/ (PTR)( &Bluetooth2Resident + 1 ),
	  /* rt_Flags		*/ RTF_NATIVE | RTF_COLDSTART,
	  /* rt_Version		*/ BLUETOOTH2_VERSION,
	  /* rt_Type		*/ NT_UNKNOWN,
	  /* rt_Pri			*/ 2, // Load BT before usb2 at pri 1
	  /* rt_Name		*/ BLUETOOTH2_NAME,
	  /* rt_IdString	*/ BLUETOOTH2_ID,
	  /* rt_Init		*/ (PTR)ROMInit };

// --

S32
_start ( void )
{
	return ( RETURN_FAIL );
}

// --
