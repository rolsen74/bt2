
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_device.h"
#include "bt2_control.h"

// --

static S32
ControlCall ( struct Bluetooth2IFace * self, struct BT2ControlCall * call )
{
	struct BT2MasterRequest request;

	request.Operation = BT2_MASTER_CONTROL;
	request.Control	  = call;

	return ( BT2_MasterCall ( (PTR)self->Data.LibBase, &request ) );
}

// --

U32
_main_control_Adapters ( struct Bluetooth2IFace * self, struct BT2ManagedAdapter * entries, U32 capacity )
{
	struct BT2ControlCall call;

	call.Operation = BT2_CONTROL_ADAPTERS;
	call.Adapters  = entries;
	call.Capacity  = entries ? capacity : 0;

	return ( ControlCall ( self, &call ) ? call.Count : 0 );
}

// --

enum BTHCIResult
_main_control_Submit ( struct Bluetooth2IFace * self, enum BT2ManagedOperation type, U32 registration,
					   const struct BTPairRequest * pair, U32 * id )
{
	struct BT2ControlCall call;

	if ( ( ! registration ) || ( ! id ) || ( ( type != BT2_MANAGED_SCAN ) && ( type != BT2_MANAGED_PAIR ) )
		 || ( ( type == BT2_MANAGED_PAIR ) && ( ! pair || ! pair->PINLength || ( pair->PINLength > BT_PIN_MAX ) ) ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	*id				  = 0;
	call.Operation	  = BT2_CONTROL_SUBMIT;
	call.JobType	  = type;
	call.Registration = registration;
	call.Pair		  = pair;

	if ( ! ControlCall ( self, &call ) )
	{
		return ( BT_HCI_NO_CONTROLLER );
	}

	if ( call.Result == BT_HCI_OK )
	{
		*id = call.ID;
	}

	return ( call.Result );
}

// --

enum BTHCIResult
_main_control_Operation ( struct Bluetooth2IFace * self, U32 id, struct BT2OperationStatus * status,
						  struct BTInquiryDevice * devices, U32 capacity )
{
	struct BT2ControlCall call;

	if ( ( ! id ) || ( ! status ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	call.Operation = BT2_CONTROL_OPERATION;
	call.ID		   = id;
	call.Status	   = status;
	call.Devices   = devices;
	call.Capacity  = devices ? capacity : 0;

	return ( ControlCall ( self, &call ) ? call.Result : BT_HCI_NO_CONTROLLER );
}

// --

enum BTHCIResult
_main_control_Cancel ( struct Bluetooth2IFace * self, U32 id )
{
	struct BT2ControlCall call;

	call.Operation = BT2_CONTROL_CANCEL;
	call.ID		   = id;

	return ( ControlCall ( self, &call ) ? call.Result : BT_HCI_NO_CONTROLLER );
}

// --

enum BTHCIResult
_main_control_StartupShare ( struct Bluetooth2IFace * self, struct BT2StartupShare * settings, U32 save )
{
	struct BT2ControlCall call;

	call.Operation	   = BT2_CONTROL_STARTUP_SHARE;
	call.ShareSettings = settings;
	call.Save		   = save;

	return ( ControlCall ( self, &call ) ? call.Result : BT_HCI_NO_CONTROLLER );
}

// --

enum BTHCIResult
_main_control_AudioOutput ( struct Bluetooth2IFace * self, struct BT2AudioOutput * settings, U32 save )
{
	struct BT2ControlCall call;

	call.Operation	   = BT2_CONTROL_AUDIO_OUTPUT;
	call.AudioSettings = settings;
	call.Save		   = save;

	return ( ControlCall ( self, &call ) ? call.Result : BT_HCI_NO_CONTROLLER );
}

// --
enum BTHCIResult
_main_control_PlayWAV ( struct Bluetooth2IFace * self, const char * path, U32 * id )
{
	struct BT2AudioOutput settings;
	struct BT2ControlCall call;
	enum BTHCIResult	  result;

	U32 length;
	S32 absolute;

	if ( ( ! path ) || ( ! id ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	*id		 = 0;
	absolute = FALSE;

	for ( length = 0; ( length < BT_SHARE_PATH_SIZE ) && path[length]; length++ )
	{
		if ( ( length ) && ( path[length] == ':' ) )
		{
			absolute = TRUE;
		}
	}

	if ( ( ! absolute ) || ( length >= BT_SHARE_PATH_SIZE ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	result = _main_control_AudioOutput ( self, &settings, FALSE );

	if ( result != BT_HCI_OK )
	{
		return ( result );
	}

	if ( ! settings.Selected )
	{
		return ( BT_HCI_NO_SAVED_KEY );
	}

	call.Operation = BT2_CONTROL_SUBMIT;
	call.JobType   = BT2_MANAGED_WAV;
	call.Audio	   = &settings;
	call.Path	   = path;
	call.Length	   = length + 1;

	if ( ! ControlCall ( self, &call ) )
	{
		return ( BT_HCI_NO_CONTROLLER );
	}

	if ( call.Result == BT_HCI_OK )
	{
		*id = call.ID;
	}

	return ( call.Result );
}

// --
U32
_main_control_Playback ( struct Bluetooth2IFace * self )
{
	struct BT2ControlCall call;

	call.Operation = BT2_CONTROL_PLAYBACK;

	return ( ControlCall ( self, &call ) ? call.ID : 0 );
}

// --
enum BTHCIResult
_main_control_PeerAction ( struct Bluetooth2IFace * self, const uint8_t * local, const uint8_t * peer,
						   enum BT2PrefsAction action )
{
	struct BT2ControlCall call;

	call.Operation	= BT2_CONTROL_PEER_ACTION;
	call.Local		= local;
	call.Peer		= peer;
	call.PeerAction = action;

	return ( ControlCall ( self, &call ) ? call.Result : BT_HCI_NO_CONTROLLER );
}

// --

enum BTHCIResult
_main_control_StartShare ( struct Bluetooth2IFace * self, const uint8_t * local, const char * path )
{
	struct BT2ControlCall call;

	call.Operation = BT2_CONTROL_SHARE_START;
	call.Local	   = local;
	call.Path	   = path;

	return ( ControlCall ( self, &call ) ? call.Result : BT_HCI_NO_CONTROLLER );
}

// --

enum BTHCIResult
_main_control_StopShare ( struct Bluetooth2IFace * self )
{
	struct BT2ControlCall call;

	call.Operation = BT2_CONTROL_SHARE_STOP;

	return ( ControlCall ( self, &call ) ? call.Result : BT_HCI_NO_CONTROLLER );
}

// --
enum BT2StartupShareState
_main_control_ShareStartupState ( struct Bluetooth2IFace * self )
{
	struct Bluetooth2Base *	  base;
	enum BT2StartupShareState state;

	base = (PTR)self->Data.LibBase;

	base->Exec->ObtainSemaphore ( &base->ControllerLock );

	state = base->ShareStartupState;

	if ( base->ShareSuppressed && state != BT2_BOOT_SHARE_DISABLED )
	{
		state = BT2_BOOT_SHARE_STOPPED;
	}

	base->Exec->ReleaseSemaphore ( &base->ControllerLock );

	return ( state );
}

// --
S32
_main_control_ShareRule ( struct Bluetooth2IFace * self, const uint8_t * local, const uint8_t * peer, S32 access )
{
	struct BT2ControlCall call;

	call.Operation = BT2_CONTROL_SHARE_RULE;
	call.Local	   = local;
	call.Peer	   = peer;
	call.Access	   = access;

	return ( ControlCall ( self, &call ) && call.Result == BT_HCI_OK ? call.Access : -1 );
}

// --
