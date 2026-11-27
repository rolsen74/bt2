
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/dos.h>
#include <dos/var.h>
#include "bt2_prefs.h"
#include "bt2_hci.h"
#include "bt2_control.h"
#include <interfaces/bluetooth2share.h>

// --

static S32
AddressMatches ( const uint8_t * a, const uint8_t * b )
{
	U32 index;

	for ( index = 0; index < 6; index++ )
	{
		if ( a[index] != b[index] )
		{
			return ( FALSE );
		}
	}

	return ( TRUE );
}

// --

static S32
HexDigit ( char digit )
{
	if ( ( digit >= '0' ) && ( digit <= '9' ) )
	{
		return ( digit - '0' );
	}

	if ( ( digit >= 'a' ) && ( digit <= 'f' ) )
	{
		return ( digit - 'a' + 10 );
	}

	return ( -1 );
}

// --

static S32
ParseBond ( const char * name, const char * prefix, uint8_t * peer )
{
	U32 index;
	S32 high;
	S32 low;

	for ( index = 0; index < 13; index++ )
	{
		if ( name[index] != prefix[index] )
		{
			return ( FALSE );
		}
	}

	for ( index = 0; index < 6; index++ )
	{
		high = HexDigit ( name[13 + index * 2] );

		if ( high < 0 )
		{
			return ( FALSE );
		}

		low = HexDigit ( name[14 + index * 2] );

		if ( low < 0 )
		{
			return ( FALSE );
		}

		peer[index] = ( high << 4 ) | low;
	}

	return ( name[25] == 0 );
}

// --

static void
AutoName ( char * name, const uint8_t * local )
{
	static const uint8_t zero[6] = { 0 };

	U32 offset;

	BT2_BondName ( name, local, zero );

	offset		   = sizeof ( BT_BOND_PREFIX ) - 1 + 13;
	name[offset++] = 'a';
	name[offset++] = 'u';
	name[offset++] = 't';
	name[offset++] = 'o';
	name[offset]   = 0;
}

// --

S32
BT2_ReadAutoPeer ( struct DOSIFace * dos, const uint8_t * local, uint8_t * peer )
{
	char name[BT_BOND_NAME_SIZE];

	AutoName ( name, local );

	return ( dos->GetVar ( name, (STRPTR)peer, 6, BT_BOND_FLAGS | GVF_DONT_NULL_TERM ) == 6 );
}

// --

void
BT2_FillAdapter ( struct Bluetooth2Base * base, struct BTController * controller, struct BT2PrefsAdapter * entry )
{
	entry->Unit		  = controller->Unit;
	entry->State	  = controller->ConnectRequested ? BT2_PREFS_CONNECTING : controller->InputState;
	entry->LastResult = controller->LastResult;
	entry->Ready	  = controller->Ready;
	entry->HID		  = ( controller->Capabilities & BT2_CAP_HID_INPUT ) != 0;
	entry->Busy		  = controller->Exclusive != NULL || controller->Pending != 0;

	base->Exec->CopyMem ( controller->Address, entry->Address, sizeof ( entry->Address ) );

	base->Exec->CopyMem ( controller->Peer, entry->Peer, sizeof ( entry->Peer ) );
}

// --

U32
_main_prefs_Adapters ( struct Bluetooth2IFace * self, struct BT2PrefsAdapter * entries, U32 capacity )
{
	struct Bluetooth2Base * base;
	struct BTController *	controller;

	U32 count;

	base  = (PTR)self->Data.LibBase;
	count = 0;

	base->Exec->ObtainSemaphore ( &base->ControllerLock );

	for ( controller = base->Controllers; controller; controller = controller->Next )
	{
		if ( ( entries ) && ( count < capacity ) )
		{
			BT2_FillAdapter ( base, controller, &entries[count] );
		}

		count++;
	}

	base->Exec->ReleaseSemaphore ( &base->ControllerLock );

	return ( count );
}

// --

static struct DOSIFace *
OpenDOS ( struct Bluetooth2Base * base, struct Library ** library )
{
	*library = base->Exec->OpenLibrary ( "dos.library", 50 );

	if ( ! *library )
	{
		return ( NULL );
	}

	return ( (PTR)base->Exec->GetInterface ( *library, "main", 1, NULL ) );
}

// --

enum BTHCIResult
_main_prefs_Peers ( struct Bluetooth2IFace * self, const uint8_t * local, struct BT2PrefsPeer * entries, U32 capacity,
					U32 * count )
{
	struct Bluetooth2Base * base;
	struct Library *		library;
	struct DOSIFace *		dos;
	struct ExamineData *	data;
	enum BTHCIResult		result;

	PTR		directory;
	uint8_t address[6];
	uint8_t automatic[6];
	char	name[BT_BOND_NAME_SIZE];
	S32		haveAuto;

	if ( ( ! local ) || ( ! count ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	base   = (PTR)self->Data.LibBase;
	*count = 0;
	result = BT_HCI_BOND_IO_ERROR;

	dos = OpenDOS ( base, &library );

	if ( ! dos )
	{
		goto done;
	}

	BT2_BondName ( name, local, local );

	base->Exec->ObtainSemaphore ( &base->BondLock );

	haveAuto = BT2_ReadAutoPeer ( dos, local, automatic );

	directory = dos->ObtainDirContextTags ( EX_StringNameInput, "ENV:Bluetooth2", EX_DataFields, EXF_TYPE | EXF_NAME, TAG_END );

	if ( directory )
	{
		while ( ( data = dos->ExamineDir ( directory ) ) )
		{
			if ( ( EXD_IS_FILE ( data ) ) && ( ParseBond ( data->Name, name + sizeof ( BT_BOND_PREFIX ) - 1, address ) ) )
			{
				if ( ( entries ) && ( *count < capacity ) )
				{
					base->Exec->CopyMem ( address, entries[*count].Address, sizeof ( address ) );

					entries[*count].AutoConnect = ( haveAuto ) && ( AddressMatches ( automatic, address ) );
				}

				++*count;
			}
		}

		if ( dos->IoErr () == ERROR_NO_MORE_ENTRIES )
		{
			result = BT_HCI_OK;
		}

		dos->ReleaseDirContext ( directory );
	}
	else if ( dos->IoErr () == ERROR_OBJECT_NOT_FOUND )
	{
		result = BT_HCI_OK;
	}

	base->Exec->ReleaseSemaphore ( &base->BondLock );

	base->Exec->DropInterface ( (PTR)dos );

done:
	if ( library )
	{
		base->Exec->CloseLibrary ( library );
	}

	return ( result );
}

// --

static S32
HasBond ( struct DOSIFace * dos, const char * name )
{
	volatile uint8_t key[BT_LINK_KEY_SIZE + 1];

	U32 index;
	S32 size;

	size = dos->GetVar ( name, (STRPTR)key, sizeof ( key ), BT_BOND_FLAGS | GVF_DONT_NULL_TERM );

	for ( index = 0; index < sizeof ( key ); index++ )
	{
		key[index] = 0;
	}

	return ( size == BT_LINK_KEY_SIZE );
}

// --

S32
BT2_HasSavedPeer ( struct DOSIFace * dos, const uint8_t * local, const uint8_t * peer )
{
	char name[BT_BOND_NAME_SIZE];

	BT2_BondName ( name, local, peer );

	return ( HasBond ( dos, name ) );
}

// --

enum BTHCIResult
BT2_PeerActionForAddress ( struct Bluetooth2Base * base, const uint8_t * local, const uint8_t * peer,
						   enum BT2PrefsAction action )
{
	struct BTController *  controller;
	struct Library *	   library;
	struct DOSIFace *	   dos;
	struct BT2StartupShare sharing;
	enum BTHCIResult	   result;

	uint8_t automatic[6];
	char	name[BT_BOND_NAME_SIZE];
	char	autoName[BT_BOND_NAME_SIZE];
	char	ruleName[BT_SHARE_RULE_NAME_SIZE];
	S32		matchesAuto;
	S32		hasBond;

	if ( ( ! local ) || ( ! peer ) || ( action > BT2_PREFS_AUTO_OFF ) )
	{
		return ( BT_HCI_INVALID_ARGUMENT );
	}

	result = BT_HCI_BOND_IO_ERROR;

	dos = OpenDOS ( base, &library );

	if ( ! dos )
	{
		goto done;
	}

	BT2_BondName ( name, local, peer );

	AutoName ( autoName, local );

	base->Exec->ObtainSemaphore ( &base->BondLock );

	matchesAuto = ( BT2_ReadAutoPeer ( dos, local, automatic ) ) && ( AddressMatches ( automatic, peer ) );
	hasBond		= HasBond ( dos, name );

	if ( ( ( action == BT2_PREFS_CONNECT ) || ( action == BT2_PREFS_AUTO_ON ) ) && ( ! hasBond ) )
	{
		result = BT_HCI_NO_SAVED_KEY;

		goto unlock;
	}

	if ( action == BT2_PREFS_AUTO_ON )
	{
		if ( BT2_ReadStartupShare ( dos, &sharing ) < 0 )
		{
			goto unlock;
		}

		if ( ( sharing.Enabled ) && ( AddressMatches ( sharing.Local, local ) ) )
		{
			result = BT_HCI_CONTROLLER_BUSY;

			goto unlock;
		}
	}

	base->Exec->ObtainSemaphore ( &base->ControllerLock );

	controller = BT2_PrefsFindAdapter ( base, local );

	if ( ! controller )
	{
		result = BT_HCI_NO_CONTROLLER;
	}
	else
	{
		result = BT_HCI_OK;

		switch ( action )
		{
			case BT2_PREFS_CONNECT:
				if ( ( ! ( controller->Capabilities & BT2_CAP_HID_INPUT ) ) || ( ! controller->PrefsWorker ) )
				{
					result = BT_HCI_UNSUPPORTED;
				}
				else if ( ( controller->Exclusive ) || ( controller->Pending ) || ( controller->InputState != BT2_PREFS_IDLE )
						  || ( controller->ConnectRequested ) )
				{
					result = BT_HCI_CONTROLLER_BUSY;
				}
				else
				{
					BT2_SetInputStopped ( base, local, FALSE );

					base->Exec->CopyMem ( peer, controller->Peer, sizeof ( controller->Peer ) );

					controller->Paused			 = FALSE;
					controller->InputAutomatic	 = FALSE;
					controller->ConnectRequested = TRUE;
					controller->LastResult		 = BT_HCI_OK;
				}
				break;

			case BT2_PREFS_DISCONNECT:
				if ( AddressMatches ( controller->Peer, peer ) )
				{
					if ( ! BT2_SetInputStopped ( base, local, TRUE ) )
					{
						result = BT_HCI_NO_MEMORY;

						break;
					}

					controller->ConnectRequested = FALSE;
					controller->Paused			 = TRUE;

					if ( ( controller->InputState != BT2_PREFS_IDLE ) && ( controller->PrefsWorker ) )
					{
						controller->InputState = BT2_PREFS_DISCONNECTING;

						base->Exec->Signal ( controller->PrefsWorker, SIGBREAKF_CTRL_C );
					}
				}
				break;

			case BT2_PREFS_FORGET:
				if ( ( AddressMatches ( controller->Peer, peer ) )
					 && ( ( controller->InputState != BT2_PREFS_IDLE ) || ( controller->ConnectRequested ) ) )
				{
					result = BT_HCI_CONTROLLER_BUSY;
				}
				break;

			default:
				break;
		}
	}

	base->Exec->ReleaseSemaphore ( &base->ControllerLock );

	if ( result != BT_HCI_OK )
	{
		goto unlock;
	}

	switch ( action )
	{
		case BT2_PREFS_FORGET:
			result = BT_HCI_BOND_IO_ERROR;

			if ( ( ! matchesAuto ) || ( dos->DeleteVar ( autoName, BT_BOND_FLAGS | GVF_SAVE_VAR ) ) )
			{
				BT2_ShareRuleName ( ruleName, local, peer );

				if ( ( dos->DeleteVar ( ruleName, BT_BOND_FLAGS | GVF_SAVE_VAR ) || dos->IoErr () == ERROR_OBJECT_NOT_FOUND )
					 && dos->DeleteVar ( name, BT_BOND_FLAGS | GVF_SAVE_VAR ) )
				{
					result = BT_HCI_OK;
				}

				BT2_ChangedPolicy ( base );
			}
			break;

		case BT2_PREFS_AUTO_ON:
			if ( dos->SetVar ( autoName, (CONST_STRPTR)peer, 6, BT_BOND_FLAGS | GVF_SAVE_VAR ) )
			{
				BT2_ChangedPolicy ( base );
			}
			else
			{
				result = BT_HCI_BOND_IO_ERROR;
			}
			break;

		case BT2_PREFS_AUTO_OFF:
			if ( ( ! matchesAuto ) || ( dos->DeleteVar ( autoName, BT_BOND_FLAGS | GVF_SAVE_VAR ) ) )
			{
				BT2_ChangedPolicy ( base );
			}
			else
			{
				result = BT_HCI_BOND_IO_ERROR;
			}
			break;

		default:
			break;
	}

unlock:
	base->Exec->ReleaseSemaphore ( &base->BondLock );

	base->Exec->DropInterface ( (PTR)dos );

done:
	if ( library )
	{
		base->Exec->CloseLibrary ( library );
	}

	return ( result );
}

// --

enum BTHCIResult
_main_prefs_Action ( struct Bluetooth2IFace * self, const uint8_t * local, const uint8_t * peer, enum BT2PrefsAction action )
{
	return ( BT2_PeerActionForAddress ( (PTR)self->Data.LibBase, local, peer, action ) );
}

// --
// The caller holds ControllerLock.
struct BTController *
BT2_PrefsFindAdapter ( struct Bluetooth2Base * base, const uint8_t * local )
{
	struct BTController * controller;

	for ( controller = base->Controllers; controller; controller = controller->Next )
	{
		if ( ( controller->Ready ) && ( AddressMatches ( controller->Address, local ) ) )
		{
			break;
		}
	}

	return ( controller );
}

// --
S32
BT2_ShareRuleForAddress ( struct Bluetooth2Base * base, const uint8_t * local, const uint8_t * peer, S32 access )
{
	struct Library *  library;
	struct DOSIFace * dos;

	char name[BT_SHARE_RULE_NAME_SIZE];
	char value[2];
	S32	 result;

	if ( ( ! local ) || ( ! peer ) || ( access < BT_SHARE_RULE_GET ) || ( access > ( BT_SHARE_READ | BT_SHARE_WRITE ) ) )
	{
		return ( -1 );
	}

	result = -1;

	dos = OpenDOS ( base, &library );

	if ( dos )
	{
		BT2_BondName ( name, local, peer );

		base->Exec->ObtainSemaphore ( &base->BondLock );

		if ( HasBond ( dos, name ) )
		{
			BT2_ShareRuleName ( name, local, peer );

			if ( access == BT_SHARE_RULE_GET )
			{
				result = 0;

				if ( ( dos->GetVar ( name, value, sizeof ( value ), BT_BOND_FLAGS | GVF_DONT_NULL_TERM ) == 1 )
					 && ( value[0] >= '0' ) && ( value[0] <= '3' ) )
				{
					result = value[0] - '0';
				}
			}
			else
			{
				value[0] = '0' + access;

				if ( dos->SetVar ( name, value, 1, BT_BOND_FLAGS | GVF_SAVE_VAR ) )
				{
					result = access;
				}
			}
		}

		base->Exec->ReleaseSemaphore ( &base->BondLock );

		base->Exec->DropInterface ( (PTR)dos );
	}

	if ( library )
	{
		base->Exec->CloseLibrary ( library );
	}

	return ( result );
}

// --
S32
_main_fileshare_Rule ( struct Bluetooth2IFace * self, const uint8_t * local, const uint8_t * peer, S32 access )
{
	return ( BT2_ShareRuleForAddress ( (PTR)self->Data.LibBase, local, peer, access ) );
}

// --
