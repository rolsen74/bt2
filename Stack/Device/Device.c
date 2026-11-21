
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

static const PTR DeviceInterfaces[] = //
	{ (PTR)ManagerTags, (PTR)MainTags, (PTR)PairingTags, (PTR)BondingTags, NULL };

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
