#ifndef BT2_DEVICE_H
#define BT2_DEVICE_H

#include <devices/timer.h>
#include <interfaces/bluetooth2control.h>
#include <interfaces/bluetooth2share.h>
#include "bt2_all.h"

struct BTAudioSource;
struct DOSIFace;
struct TimerIFace;

struct BTDeadline
{
	struct TimerIFace *Interface;
	struct TimeRequest *Request;
	struct MsgPort *Port;
	struct TimeVal Start;
	U32 Seconds;
	U32 Started;
};

struct BTFileSource
{
	struct DOSIFace *DOS;
	int32_t *FileError;
	BPTR Handle;
};

struct BTFileSink
{
	struct DOSIFace *DOS;
	struct BTRFCOMMCapture *Request;
	BPTR Handle;
};

extern const PTR DeviceInterfaces[];

PTR ROMInit( PTR dummy, PTR seglist, struct ExecBase *sysbase );
PTR ROMExit( struct Bluetooth2Base *base );

void _manager_Open( struct DeviceManagerInterface *self, struct IORequest *ioreq,
	U32 unit, U32 flags );
PTR _manager_Close( struct DeviceManagerInterface *self, struct IORequest *ioreq );
PTR _manager_Expunge( struct DeviceManagerInterface *self );
void _manager_BeginIO( struct DeviceManagerInterface *self, struct IORequest *ioreq );
void _manager_AbortIO( struct DeviceManagerInterface *self,
	struct IORequest *ioreq );

U32 _main_Obtain( struct Interface *self );
U32 _main_Release( struct Interface *self );

enum BTHCIResult _main_controller_ResetController( struct Bluetooth2IFace *self,
	const struct BTHCITransport *transport, struct BTHCIError *error );
enum BTHCIResult _main_controller_ReadControllerInfo( struct Bluetooth2IFace *self,
	const struct BTHCITransport *transport, struct BTControllerInfo *info, struct BTHCIError *error );
S32 _main_controller_AddController( struct Bluetooth2IFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location );
void _main_controller_RemoveController( struct Bluetooth2IFace *self, U32 unit, struct MsgPort *port );
enum BTHCIResult _main_controller_ProbeController( struct Bluetooth2IFace *self, U32 unit, struct BTControllerLocation *location,
	struct BTControllerInfo *info, struct BTHCIError *error );
enum BTHCIResult _main_controller_DiscoverController( struct Bluetooth2IFace *self, U32 unit, struct BTControllerLocation *location,
	struct BTInquiryResult *inquiry, struct BTHCIError *error );
enum BTHCIResult _main_controller_Inquiry( struct Bluetooth2IFace *self, const struct BTHCITransport *transport,
	struct BTInquiryResult *inquiry, struct BTHCIError *error );
S32 _main_controller_AddDiscoveryController( struct Bluetooth2IFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location );

enum BTHCIResult _main_pairing_PairController( struct Bluetooth2IFace *self, U32 unit,
	const struct BTPairRequest *pair, struct BTHCIError *error );
enum BTHCIResult _main_pairing_Pair( struct Bluetooth2IFace *self, const struct BTHCITransport *transport,
	const struct BTPairRequest *pair, struct BTHCIError *error );
S32 _main_pairing_AddController( struct Bluetooth2IFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location );

enum BTHCIResult _main_bonding_BondController( struct Bluetooth2IFace *self, U32 unit,
	struct BTBondRequest *bond, struct BTHCIError *error );
enum BTHCIResult _main_bonding_Bond( struct Bluetooth2IFace *self, const struct BTHCITransport *transport,
	struct BTBondRequest *bond, struct BTHCIError *error );
S32 _main_bonding_AddController( struct Bluetooth2IFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location );

enum BTHCIResult _main_names_ReadName( struct Bluetooth2IFace *self, U32 unit,
	struct BTRemoteName *remote, struct BTHCIError *error );
enum BTHCIResult _main_names_ReadRemoteName( struct Bluetooth2IFace *self, const struct BTHCITransport *transport,
	struct BTRemoteName *remote, struct BTHCIError *error );
S32 _main_names_AddController( struct Bluetooth2IFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location );

enum BTHCIResult _main_visibility_MakeVisible( struct Bluetooth2IFace *self, U32 unit,
	const struct BTVisibility *visibility, struct BTHCIError *error );
enum BTHCIResult _main_visibility_RunVisible( struct Bluetooth2IFace *self, const struct BTHCITransport *transport,
	const struct BTVisibility *visibility, struct BTHCIError *error );
S32 _main_visibility_AddController( struct Bluetooth2IFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location );

enum BTHCIResult _main_l2cap_EchoController( struct Bluetooth2IFace *self, U32 unit, const uint8_t *address, struct BTHCIError *error );
enum BTHCIResult _main_l2cap_Echo( struct Bluetooth2IFace *self, const struct BTACLTransport *transport,
	const uint8_t *address, struct BTHCIError *error );
S32 _main_l2cap_AddController( struct Bluetooth2IFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location );

enum BTHCIResult _main_sdp_BrowseController( struct Bluetooth2IFace *self, U32 unit, struct BTServiceQuery *query, struct BTHCIError *error );
enum BTHCIResult _main_sdp_Browse( struct Bluetooth2IFace *self,
	const struct BTACLTransport *transport, struct BTServiceQuery *query, struct BTHCIError *error );
S32 _main_sdp_AddController( struct Bluetooth2IFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location );

enum BTHCIResult _main_rfcomm_TestController( struct Bluetooth2IFace *self, U32 unit, struct BTRFCOMMTest *test, struct BTHCIError *error );
enum BTHCIResult _main_rfcomm_Test( struct Bluetooth2IFace *self, const struct BTACLTransport *transport,
	struct BTRFCOMMTest *test, struct BTHCIError *error );
S32 _main_rfcomm_AddController( struct Bluetooth2IFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location );

enum BTHCIResult _main_serial_ExchangeController( struct Bluetooth2IFace *self, U32 unit,
	struct BTRFCOMMTransfer *transfer, struct BTHCIError *error );
enum BTHCIResult _main_serial_Exchange( struct Bluetooth2IFace *self, const struct BTACLTransport *transport,
	struct BTRFCOMMTransfer *transfer, struct BTHCIError *error );
S32 _main_serial_AddController( struct Bluetooth2IFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location );

enum BTHCIResult _main_serialfile_SendController( struct Bluetooth2IFace *self, U32 unit,
	struct BTRFCOMMFile *file, struct BTHCIError *error );
enum BTHCIResult _main_serialfile_Send( struct Bluetooth2IFace *self, const struct BTACLTransport *transport,
	struct BTRFCOMMFile *file, struct BTHCIError *error );
S32 _main_serialfile_AddController( struct Bluetooth2IFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location );

enum BTHCIResult _main_serialcapture_ReceiveController( struct Bluetooth2IFace *self, U32 unit,
	struct BTRFCOMMCapture *capture, struct BTHCIError *error );
enum BTHCIResult _main_serialcapture_Receive( struct Bluetooth2IFace *self, const struct BTACLTransport *transport,
	struct BTRFCOMMCapture *capture, struct BTHCIError *error );
S32 _main_serialcapture_AddController( struct Bluetooth2IFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location );

enum BTHCIResult _main_objectpush_PushController( struct Bluetooth2IFace *self, U32 unit,
	struct BTObjectPush *push, struct BTHCIError *error );
enum BTHCIResult _main_objectpush_Push( struct Bluetooth2IFace *self, const struct BTACLTransport *transport,
	struct BTObjectPush *push, struct BTHCIError *error );
S32 _main_objectpush_AddController( struct Bluetooth2IFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location );

enum BTHCIResult _main_le_RunController( struct Bluetooth2IFace *self, U32 unit,
	struct BTLERequest *request, struct BTHCIError *error );
enum BTHCIResult _main_le_Run( struct Bluetooth2IFace *self, const struct BTHCITransport *transport,
	struct BTLERequest *request, struct BTHCIError *error );
S32 _main_le_AddController( struct Bluetooth2IFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location, U32 acl );

enum BTHCIResult _main_a2dp_PlayController( struct Bluetooth2IFace *self, U32 unit,
	struct BTAudioPlayback *audio, struct BTHCIError *error );
enum BTHCIResult _main_a2dp_Play( struct Bluetooth2IFace *self, const struct BTACLTransport *transport,
	struct BTAudioPlayback *audio, struct BTHCIError *error );
S32 _main_a2dp_AddController( struct Bluetooth2IFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location );

enum BTHCIResult _main_pcm_PlayController( struct Bluetooth2IFace *self, U32 unit,
	struct BTAudioPCM *pcm, struct BTHCIError *error );
enum BTHCIResult _main_pcm_Play( struct Bluetooth2IFace *self, const struct BTACLTransport *transport,
	struct BTAudioPCM *pcm, struct BTHCIError *error );
S32 _main_pcm_AddController( struct Bluetooth2IFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location );

enum BTHCIResult _main_hid_MonitorController( struct Bluetooth2IFace *self, U32 unit,
	struct BTHIDMonitor *monitor, struct BTHCIError *error );
enum BTHCIResult _main_hid_Monitor( struct Bluetooth2IFace *self, const struct BTACLTransport *transport,
	struct BTHIDMonitor *monitor, struct BTHCIError *error );
S32 _main_hid_AddController( struct Bluetooth2IFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location );

enum BTHCIResult _main_hidinput_ConnectController( struct Bluetooth2IFace *self, U32 unit,
	struct BTHIDInput *input, struct BTHCIError *error );
enum BTHCIResult _main_hidinput_Connect( struct Bluetooth2IFace *self, const struct BTACLTransport *transport,
	struct BTHIDInput *input, struct BTHCIError *error );
S32 _main_hidinput_AddController( struct Bluetooth2IFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location );

S32 _main_fileshare_AddController( struct Bluetooth2IFace *self, struct MsgPort *port,
	const struct BTControllerLocation *location );

U32 _main_control_Adapters( struct Bluetooth2IFace *self, struct BT2ManagedAdapter *entries, U32 capacity );
enum BTHCIResult _main_control_Submit( struct Bluetooth2IFace *self, enum BT2ManagedOperation type, U32 registration,
	const struct BTPairRequest *pair, U32 *id );
enum BTHCIResult _main_control_Operation( struct Bluetooth2IFace *self, U32 id, struct BT2OperationStatus *status,
	struct BTInquiryDevice *devices, U32 capacity );
enum BTHCIResult _main_control_Cancel( struct Bluetooth2IFace *self, U32 id );
enum BTHCIResult _main_control_StartupShare( struct Bluetooth2IFace *self, struct BT2StartupShare *settings, U32 save );
enum BTHCIResult _main_control_AudioOutput( struct Bluetooth2IFace *self, struct BT2AudioOutput *settings, U32 save );
enum BTHCIResult _main_control_PlayWAV( struct Bluetooth2IFace *self, const char *path, U32 *id );
U32 _main_control_Playback( struct Bluetooth2IFace *self );
enum BTHCIResult _main_control_PeerAction( struct Bluetooth2IFace *self, const uint8_t *local, const uint8_t *peer,
	enum BT2PrefsAction action );
enum BTHCIResult _main_control_StartShare( struct Bluetooth2IFace *self, const uint8_t *local, const char *path );
enum BTHCIResult _main_control_StopShare( struct Bluetooth2IFace *self );
enum BT2StartupShareState _main_control_ShareStartupState( struct Bluetooth2IFace *self );
S32 _main_control_ShareRule( struct Bluetooth2IFace *self, const uint8_t *local, const uint8_t *peer, S32 access );

void NSCmd_DeviceQuery( struct IORequest *ioreq );
void EnumUnits( struct Bluetooth2Base *base, struct IORequest *ioreq );

U32 LECapabilities( U32 acl );
S32 RegisterController( struct Bluetooth2Base *base, struct MsgPort *port,
	const struct BTControllerLocation *location, S32 capabilities );

int DeadlineExpired( PTR context );
enum BTHCIResult OpenDeadline( struct Bluetooth2Base *base, struct BTDeadline *timer, U32 seconds );
void CloseDeadline( struct Bluetooth2Base *base, struct BTDeadline *timer );

enum BTHCIResult PlayAudioSource( struct Bluetooth2Base *base, const struct BTACLTransport *transport,
	struct BTAudioSource *source, struct BTHCIError *error );
int32_t ReadSerialFile( void *context, uint8_t *data, uint32_t capacity );
enum BTHCIResult ReceiveFile( struct Bluetooth2Base *base, const struct BTACLTransport *transport,
	struct BTFileSink *sink, struct BTHCIError *error );

enum BTHCIResult RequestHID( struct Bluetooth2Base *base, U32 unit, struct BTHIDMonitor *monitor, struct BTHCIError *error );
enum BTHCIResult RunRFCOMMWithBond( struct Bluetooth2Base *base, const struct BTACLTransport *transport,
	struct BTRFCOMMTest *test, struct BTRFCOMMTransfer *transfer, struct BTHCIError *error );
S32 ValidHIDInput( const struct BTHIDInput *input );
S32 ValidHIDMonitor( const struct BTHIDMonitor *monitor );
S32 ValidPCM( const struct BTAudioPCM *pcm );

#endif
