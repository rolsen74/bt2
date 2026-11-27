#ifndef BT2_CONTROL_H
#define BT2_CONTROL_H

#include "bt2_prefs.h"
#include <interfaces/bluetooth2control.h>

struct DOSIFace;

enum BT2ControlOperation
{
	BT2_CONTROL_ADAPTERS,
	BT2_CONTROL_SUBMIT,
	BT2_CONTROL_OPERATION,
	BT2_CONTROL_CANCEL,
	BT2_CONTROL_PLAYBACK,
	BT2_CONTROL_POLICY_FIRST,
	BT2_CONTROL_STARTUP_SHARE = BT2_CONTROL_POLICY_FIRST,
	BT2_CONTROL_AUDIO_OUTPUT,
	BT2_CONTROL_PEER_ACTION,
	BT2_CONTROL_SHARE_START,
	BT2_CONTROL_SHARE_STOP,
	BT2_CONTROL_SHARE_RULE,
};

struct BT2ControlCall
{
	enum BT2ControlOperation Operation;
	enum BT2ManagedOperation JobType;
	enum BTHCIResult Result;
	struct BT2ManagedAdapter *Adapters;
	struct BT2OperationStatus *Status;
	struct BTInquiryDevice *Devices;
	const struct BTPairRequest *Pair;
	const struct BT2AudioOutput *Audio;
	const char *Path;
	const uint8_t *Local;
	const uint8_t *Peer;
	struct BT2StartupShare *ShareSettings;
	struct BT2AudioOutput *AudioSettings;
	enum BT2PrefsAction PeerAction;
	U32 Save;
	S32 Access;
	U32 Length;
	U32 Capacity;
	U32 Count;
	U32 Registration;
	U32 ID;
};

struct BT2ManagedJob
{
	struct BTControllerProbe Probe;
	struct BT2ManagedJob *Next;
	struct BTController *Controller;
	struct BTHCIError Error;
	struct BTInquiryResult Inquiry;
	struct BTBondRequest Bond;
	struct BTAudioPlayback Audio;
	char Path[BT_SHARE_PATH_SIZE];
	struct BTInquiryDevice Devices[BT2_SCAN_CAPACITY];
	U32 ID;
};

void BT2_ClearJobPIN( struct BT2ManagedJob *job );
void BT2_FillAdapter( struct Bluetooth2Base *base, struct BTController *controller, struct BT2PrefsAdapter *entry );
enum BTHCIResult BT2_StartupShareSettings( struct Bluetooth2Base *base, struct BT2StartupShare *settings, U32 save );
enum BTHCIResult BT2_AudioOutputSettings( struct Bluetooth2Base *base, struct BT2AudioOutput *settings, U32 save );
enum BTHCIResult BT2_RunShareForAddress( struct Bluetooth2Base *base, const uint8_t *local,
	const char *path, S32 automatic, U32 revision );
enum BTHCIResult BT2_PeerActionForAddress( struct Bluetooth2Base *base, const uint8_t *local,
	const uint8_t *peer, enum BT2PrefsAction action );
S32 BT2_ShareRuleForAddress( struct Bluetooth2Base *base, const uint8_t *local, const uint8_t *peer, S32 access );
S32 BT2_HasSavedPeer( struct DOSIFace *dos, const uint8_t *local, const uint8_t *peer );
void BT2_StopManagedShare( struct Bluetooth2Base *base );
S32 BT2_ReadStartupShare( struct DOSIFace *dos, struct BT2StartupShare *settings );
void BT2_ChangedPolicy( struct Bluetooth2Base *base );
S32 BT2_SetInputStopped( struct Bluetooth2Base *base, const uint8_t *local, S32 stop );

#endif
