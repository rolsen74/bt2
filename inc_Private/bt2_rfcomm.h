#ifndef BT2_RFCOMM_H
#define BT2_RFCOMM_H

#include "bt2_l2cap.h"

#define BT_RFCOMM_F 0x10
#define BT_RFCOMM_SABM 0x2f
#define BT_RFCOMM_DISC 0x43
#define BT_RFCOMM_UA 0x63
#define BT_RFCOMM_DM 0x0f
#define BT_RFCOMM_UIH 0xef

struct BTRFCOMMStream
{
	void *Context;
	int32_t (*Read)( void *context, uint8_t *data, uint32_t capacity );
	struct BTRFCOMMFile *File;
};

struct BTRFCOMMSink
{
	void *Context;
	int32_t (*Write)( void *context, const uint8_t *data, uint32_t length );
	struct BTRFCOMMCapture *Capture;
};

struct BTOBEXPush;

struct BTRFCOMMSession
{
	struct BTL2CAPLink *Link;
	struct BTRFCOMMTransfer *Transfer;
	struct BTRFCOMMStream *Stream;
	struct BTRFCOMMSink *Sink;
	struct BTOBEXPush *Push;
	uint16_t LocalCID;
	uint16_t RemoteCID;
	uint16_t SendCredits;
	uint8_t DLCI;
	uint8_t FrameSize;
	uint8_t CreditMode;
	uint8_t PeerModemSeen;
	uint8_t PeerPaused;
	uint8_t MuxPaused;
};

struct BTRFCOMMFrame
{
	const uint8_t *Data;
	uint16_t Length;
	uint8_t DLCI;
	uint8_t Type;
	uint8_t Final;
	uint8_t Credits;
};

enum BTHCIResult BT2_WriteRFCOMM( struct BTRFCOMMSession *session, uint8_t dlci, uint8_t control,
	const uint8_t *data, uint8_t size, struct BTHCIError *error );
int BT2_DecodeRFCOMM( const struct BTL2CAPPacket *pdu, uint8_t creditMode, struct BTRFCOMMFrame *frame );

enum BTRFCOMMWait
{
	BT_RFCOMM_WAIT_ACK,
	BT_RFCOMM_WAIT_PN,
	BT_RFCOMM_WAIT_MSC,
	BT_RFCOMM_WAIT_WRITABLE,
	BT_RFCOMM_WAIT_REPLY,
	BT_RFCOMM_WAIT_CAPTURE,
	BT_RFCOMM_WAIT_OBEX,
};

enum BTHCIResult BT2_WaitRFCOMM( struct BTRFCOMMSession *session, enum BTRFCOMMWait condition, uint8_t dlci,
	struct BTHCIError *error );
enum BTHCIResult BT2_ExchangeSerialData( struct BTRFCOMMSession *session, struct BTHCIError *error );

enum BTHCIResult BT2_WriteSerialData( struct BTRFCOMMSession *session, const uint8_t *data, uint32_t length,
	uint32_t *sent, struct BTHCIError *error );
enum BTHCIResult BT2_SendSerialStream( struct BTRFCOMMSession *session, struct BTHCIError *error );
enum BTHCIResult BT2_SendRFCOMMStream( const struct BTACLTransport *transport, struct BTRFCOMMStream *stream,
	const uint8_t *key, struct BTHCIError *error );

enum BTHCIResult BT2_GrantRFCOMMCredit( struct BTRFCOMMSession *session, struct BTHCIError *error );
enum BTHCIResult BT2_CaptureSerialFrame( struct BTRFCOMMSession *session, const struct BTRFCOMMFrame *frame,
	struct BTHCIError *error );
enum BTHCIResult BT2_CaptureRFCOMMStream( const struct BTACLTransport *transport, struct BTRFCOMMSink *sink,
	const uint8_t *key, struct BTHCIError *error );

#endif
