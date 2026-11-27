#ifndef BT2_SHARE_H
#define BT2_SHARE_H

#include <interfaces/dos.h>
#include <interfaces/bluetooth2share.h>
#include "bt2_prefs.h"
#include "bt2_rfcomm.h"

#define BT_SHARE_CHANNEL 12
#define BT_SHARE_DLCI ( BT_SHARE_CHANNEL * 2 )
#define BT_SHARE_PACKET 512
#define BT_SHARE_NAME 106
#define BT_SHARE_TEMP ".bt2-upload-"

struct BTShareServer
{
	struct Bluetooth2Base *Base;
	struct DOSIFace *DOS;
	struct BTFileShare *Request;
	BPTR Root;
};

struct BTShareSession
{
	struct BTShareServer *Server;
	struct BTRFCOMMSession RFCOMM;
	uint8_t Local[6];
	uint8_t Peer[6];
	uint8_t Key[16];
	uint8_t Access;
	uint8_t OBEXConnected;
	uint8_t MuxOpen;
	uint8_t DLCOpen;
	uint16_t Limit;
	uint16_t Used;
	uint16_t PendingLength;
	uint8_t Pending[BT_SHARE_PACKET];
	uint8_t Input[BT_SHARE_PACKET];
	BPTR File;
	BPTR Temp;
	char TempName[32];
	char Name[BT_SHARE_NAME + 1];
	uint32_t Remaining;
	uint8_t HasLength;
	uint8_t Getting;
	PTR Listing;
	char Text[BT_SHARE_NAME * 6 + 80];
	uint16_t TextOffset;
	uint16_t TextLength;
};

enum BTHCIResult _main_sharecontrol_Start( struct Bluetooth2IFace *self, const uint8_t *local, const char *path );
void _main_sharecontrol_Stop( struct Bluetooth2IFace *self );
void _main_sharecontrol_Status( struct Bluetooth2IFace *self, struct BTShareStatus *status );
enum BTHCIResult BT2_RequestShare( struct Bluetooth2Base *base, struct BTController *controller,
	struct BTFileShare *share, struct BTHCIError *error );
enum BTHCIResult _main_fileshare_Listen( struct Bluetooth2IFace *self,
	struct BTFileShare *share, struct BTHCIError *error );
enum BTHCIResult _main_fileshare_Serve( struct Bluetooth2IFace *self,
	const struct BTACLTransport *transport, struct BTFileShare *share, struct BTHCIError *error );
enum BTHCIResult BT2_RunShare( struct BTShareServer *server, const struct BTACLTransport *transport, struct BTHCIError *error );
S32 _main_fileshare_Rule( struct Bluetooth2IFace *self, const uint8_t *local, const uint8_t *peer, S32 access );
uint8_t BT2_ShareAccess( struct BTShareSession *session, uint8_t *key );
S32 BT2_ShareEncrypted( struct BTL2CAPLink *link, struct BTHCIError *error );
enum BTHCIResult BT2_ShareTransport( struct BTShareSession *session, struct BTL2CAPLink *link, struct BTHCIError *error );
enum BTHCIResult BT2_ShareSDP( struct BTL2CAPLink *link, uint16_t cid, const uint8_t *packet,
	uint16_t length, struct BTHCIError *error );
enum BTHCIResult BT2_ShareRFCOMM( struct BTShareSession *session, const struct BTL2CAPPacket *packet, struct BTHCIError *error );
enum BTHCIResult BT2_ShareOBEX( struct BTShareSession *session, const uint8_t *input, uint16_t length, struct BTHCIError *error );
enum BTHCIResult BT2_ShareSend( struct BTShareSession *session, const uint8_t *data,
	uint16_t length, struct BTHCIError *error );
void BT2_ShareClose( struct BTShareSession *session );
S32 BT2_ShareOpenRead( struct BTShareSession *session, const char *name, U32 *size );
S32 BT2_ShareOpenUpload( struct BTShareSession *session, const char *name );
S32 BT2_ShareFinishUpload( struct BTShareSession *session );
S32 BT2_ShareOpenListing( struct BTShareSession *session );
S32 BT2_ShareReadListing( struct BTShareSession *session, uint8_t *data, U32 capacity );
S32 BT2_ShareValidName( const char *name );

#endif
