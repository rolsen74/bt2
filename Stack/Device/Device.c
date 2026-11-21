
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_all.h"

// --

#include "_man_Open.c"
#include "Device_Shutdown.c"
#include "_man_Close.c"
#include "_man_Expunge.c"
#include "_man_BeginIO.c"
#include "_man_AbortIO.c"

#include "_main_Obtain.c"
#include "_main_Release.c"
#include "_main_ResetController.c"
#include "_main_ReadControllerInfo.c"
#include "_main_AddController.c"
#include "_main_RemoveController.c"
#include "_main_ProbeController.c"
#include "_main_DiscoverController.c"
#include "_main_Inquiry.c"
#include "_main_PairController.c"
#include "_main_BondController.c"
#include "_main_ReadRemoteName.c"
#include "Deadline.c"
#include "_main_MakeVisible.c"
#include "_le_Run.c"
#include "_main_L2CAPEcho.c"
#include "_main_BrowseServices.c"
#include "_main_TestRFCOMM.c"
#include "_main_ExchangeRFCOMM.c"
#include "_main_SendRFCOMMFile.c"
#include "_main_CaptureRFCOMMFile.c"
#include "_main_PushObject.c"
#include "_a2dp_Play.c"
#include "_pcm_Play.c"

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
	{ { MIT_Name, ( Tag ) "__device" }, { MIT_VectorTable, (Tag)ManagerVectors }, { MIT_Version, 1 }, { TAG_END, 0 } };

static const PTR MainVectors[] = //
	{ (PTR)_main_Obtain,
	  (PTR)_main_Release,
	  NULL,
	  NULL,
	  (PTR)_main_ResetController,
	  (PTR)_main_ReadControllerInfo,
	  (PTR)_main_AddController,
	  (PTR)_main_RemoveController,
	  (PTR)_main_ProbeController,
	  (PTR)_main_DiscoverController,
	  (PTR)_main_Inquiry,
	  (PTR)_main_AddDiscoveryController,
	  (PTR)-1 };

static const struct TagItem MainTags[] = //
	{ { MIT_Name, ( Tag ) "main" },
	  { MIT_VectorTable, (Tag)MainVectors },
	  { MIT_Version, BLUETOOTH2_INTERFACE_VERSION },
	  { TAG_END, 0 } };

static const PTR PairingVectors[] = //
	{ (PTR)_main_Obtain,
	  (PTR)_main_Release,
	  NULL,
	  NULL,
	  (PTR)_pairing_PairController,
	  (PTR)_pairing_Pair,
	  (PTR)_pairing_AddController,
	  (PTR)-1 };

static const struct TagItem PairingTags[] = //
	{ { MIT_Name, ( Tag ) BLUETOOTH2_PAIRING_INTERFACE },
	  { MIT_VectorTable, (Tag)PairingVectors },
	  { MIT_Version, BLUETOOTH2_PAIRING_VERSION },
	  { TAG_END, 0 } };

static const PTR BondingVectors[] = //
	{ (PTR)_main_Obtain,
	  (PTR)_main_Release,
	  NULL,
	  NULL,
	  (PTR)_bonding_BondController,
	  (PTR)_bonding_Bond,
	  (PTR)_bonding_AddController,
	  (PTR)-1 };

static const struct TagItem BondingTags[] = //
	{ { MIT_Name, ( Tag ) BLUETOOTH2_BONDING_INTERFACE },
	  { MIT_VectorTable, (Tag)BondingVectors },
	  { MIT_Version, BLUETOOTH2_BONDING_VERSION },
	  { TAG_END, 0 } };

static const PTR NamesVectors[] = //
	{ (PTR)_main_Obtain,
	  (PTR)_main_Release,
	  NULL,
	  NULL,
	  (PTR)_names_ReadName,
	  (PTR)_names_ReadRemoteName,
	  (PTR)_names_AddController,
	  (PTR)-1 };

static const struct TagItem NamesTags[] = //
	{ { MIT_Name, ( Tag ) BLUETOOTH2_NAMES_INTERFACE },
	  { MIT_VectorTable, (Tag)NamesVectors },
	  { MIT_Version, BLUETOOTH2_NAMES_VERSION },
	  { TAG_END, 0 } };

static const PTR VisibilityVectors[] = //
	{ (PTR)_main_Obtain,
	  (PTR)_main_Release,
	  NULL,
	  NULL,
	  (PTR)_visibility_MakeVisible,
	  (PTR)_visibility_RunVisible,
	  (PTR)_visibility_AddController,
	  (PTR)-1 };

static const struct TagItem VisibilityTags[] = //
	{ { MIT_Name, ( Tag ) BLUETOOTH2_VISIBILITY_INTERFACE },
	  { MIT_VectorTable, (Tag)VisibilityVectors },
	  { MIT_Version, BLUETOOTH2_VISIBILITY_VERSION },
	  { TAG_END, 0 } };

static const PTR L2CAPVectors[] = //
	{ (PTR)_main_Obtain,
	  (PTR)_main_Release,
	  NULL,
	  NULL,
	  (PTR)_l2cap_EchoController,
	  (PTR)_l2cap_Echo,
	  (PTR)_l2cap_AddController,
	  (PTR)-1 };

static const struct TagItem L2CAPTags[] = //
	{ { MIT_Name, ( Tag ) BLUETOOTH2_L2CAP_INTERFACE },
	  { MIT_VectorTable, (Tag)L2CAPVectors },
	  { MIT_Version, BLUETOOTH2_L2CAP_VERSION },
	  { TAG_END, 0 } };

static const PTR SDPVectors[] = //
	{ (PTR)_main_Obtain,
	  (PTR)_main_Release,
	  NULL,
	  NULL,
	  (PTR)_sdp_BrowseController,
	  (PTR)_sdp_Browse,
	  (PTR)_sdp_AddController,
	  (PTR)-1 };

static const struct TagItem SDPTags[] = //
	{ { MIT_Name, ( Tag ) BLUETOOTH2_SDP_INTERFACE },
	  { MIT_VectorTable, (Tag)SDPVectors },
	  { MIT_Version, BLUETOOTH2_SDP_VERSION },
	  { TAG_END, 0 } };

static const PTR RFCOMMVectors[] = //
	{ (PTR)_main_Obtain,
	  (PTR)_main_Release,
	  NULL,
	  NULL,
	  (PTR)_rfcomm_TestController,
	  (PTR)_rfcomm_Test,
	  (PTR)_rfcomm_AddController,
	  (PTR)-1 };

static const struct TagItem RFCOMMTags[] = //
	{ { MIT_Name, ( Tag ) BLUETOOTH2_RFCOMM_INTERFACE },
	  { MIT_VectorTable, (Tag)RFCOMMVectors },
	  { MIT_Version, BLUETOOTH2_RFCOMM_VERSION },
	  { TAG_END, 0 } };

static const PTR SerialVectors[] = //
	{ (PTR)_main_Obtain,
	  (PTR)_main_Release,
	  NULL,
	  NULL,
	  (PTR)_serial_ExchangeController,
	  (PTR)_serial_Exchange,
	  (PTR)_serial_AddController,
	  (PTR)-1 };

static const struct TagItem SerialTags[] = //
	{ { MIT_Name, ( Tag ) BLUETOOTH2_SERIAL_INTERFACE },
	  { MIT_VectorTable, (Tag)SerialVectors },
	  { MIT_Version, BLUETOOTH2_SERIAL_VERSION },
	  { TAG_END, 0 } };

static const PTR SerialFileVectors[] = //
	{ (PTR)_main_Obtain,
	  (PTR)_main_Release,
	  NULL,
	  NULL,
	  (PTR)_serialfile_SendController,
	  (PTR)_serialfile_Send,
	  (PTR)_serialfile_AddController,
	  (PTR)-1 };

static const struct TagItem SerialFileTags[] = //
	{ { MIT_Name, ( Tag ) BLUETOOTH2_SERIALFILE_INTERFACE },
	  { MIT_VectorTable, (Tag)SerialFileVectors },
	  { MIT_Version, BLUETOOTH2_SERIALFILE_VERSION },
	  { TAG_END, 0 } };

static const PTR SerialCaptureVectors[] = //
	{ (PTR)_main_Obtain,
	  (PTR)_main_Release,
	  NULL,
	  NULL,
	  (PTR)_serialcapture_ReceiveController,
	  (PTR)_serialcapture_Receive,
	  (PTR)_serialcapture_AddController,
	  (PTR)-1 };

static const struct TagItem SerialCaptureTags[] = //
	{ { MIT_Name, ( Tag ) BLUETOOTH2_CAPTURE_INTERFACE },
	  { MIT_VectorTable, (Tag)SerialCaptureVectors },
	  { MIT_Version, BLUETOOTH2_CAPTURE_VERSION },
	  { TAG_END, 0 } };

static const PTR ObjectPushVectors[] = //
	{ (PTR)_main_Obtain,
	  (PTR)_main_Release,
	  NULL,
	  NULL,
	  (PTR)_objectpush_PushController,
	  (PTR)_objectpush_Push,
	  (PTR)_objectpush_AddController,
	  (PTR)-1 };

static const struct TagItem ObjectPushTags[] = //
	{ { MIT_Name, ( Tag ) BLUETOOTH2_OBEX_INTERFACE },
	  { MIT_VectorTable, (Tag)ObjectPushVectors },
	  { MIT_Version, BLUETOOTH2_OBEX_VERSION },
	  { TAG_END, 0 } };

static const PTR LEVectors[] = //
	{ (PTR)_main_Obtain,
	  (PTR)_main_Release,
	  NULL,
	  NULL,
	  (PTR)_le_RunController,
	  (PTR)_le_Run,
	  (PTR)_le_AddController,
	  (PTR)-1 };

static const struct TagItem LETags[] = //
	{ { MIT_Name, ( Tag ) BLUETOOTH2_LE_INTERFACE },
	  { MIT_VectorTable, (Tag)LEVectors },
	  { MIT_Version, BLUETOOTH2_LE_VERSION },
	  { TAG_END, 0 } };

static const PTR A2DPVectors[] = //
	{ (PTR)_main_Obtain,
	  (PTR)_main_Release,
	  NULL,
	  NULL,
	  (PTR)_a2dp_PlayController,
	  (PTR)_a2dp_Play,
	  (PTR)_a2dp_AddController,
	  (PTR)-1 };

static const struct TagItem A2DPTags[] = //
	{ { MIT_Name, ( Tag ) BLUETOOTH2_A2DP_INTERFACE },
	  { MIT_VectorTable, (Tag)A2DPVectors },
	  { MIT_Version, BLUETOOTH2_A2DP_VERSION },
	  { TAG_END, 0 } };

static const PTR PCMVectors[] = //
	{ (PTR)_main_Obtain,
	  (PTR)_main_Release,
	  NULL,
	  NULL,
	  (PTR)_pcm_PlayController,
	  (PTR)_pcm_Play,
	  (PTR)_pcm_AddController,
	  (PTR)-1 };

static const struct TagItem PCMTags[] = //
	{ { MIT_Name, ( Tag ) BLUETOOTH2_PCM_INTERFACE },
	  { MIT_VectorTable, (Tag)PCMVectors },
	  { MIT_Version, BLUETOOTH2_PCM_VERSION },
	  { TAG_END, 0 } };

static const PTR DeviceInterfaces[] = //
	{ (PTR)ManagerTags, (PTR)MainTags, (PTR)PairingTags, (PTR)BondingTags, (PTR)NamesTags,
	  (PTR)VisibilityTags, (PTR)L2CAPTags, (PTR)SDPTags, (PTR)RFCOMMTags, (PTR)SerialTags,
	  (PTR)SerialFileTags, (PTR)SerialCaptureTags, (PTR)ObjectPushTags, (PTR)LETags, (PTR)A2DPTags, (PTR)PCMTags, NULL };

// --

#include "Device_Startup.c"

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
