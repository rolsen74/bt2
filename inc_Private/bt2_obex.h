#ifndef BT2_OBEX_H
#define BT2_OBEX_H

#include "bt2_rfcomm.h"

#define BT_OBEX_MAX_PACKET 1024
#define BT_OBEX_HEADER 3

struct BTOBEXPush
{
	struct BTObjectPush *Request;
	void *Context;
	int32_t (*Read)( void *context, uint8_t *data, uint32_t capacity );
	const char *Name;
	uint32_t Size;
	uint32_t ReplyUsed;
	uint32_t ReplySize;
	uint16_t PacketLimit;
	uint8_t Reply[BT_OBEX_MAX_PACKET];
	uint8_t ConnectionID[4];
	uint8_t HasConnectionID;
	uint8_t AwaitingReply;
	uint8_t Accepted;
};

static inline uint16_t
BT_OBEXReadBE16( const uint8_t *bytes )
{
	return ( ( (uint16_t) bytes[0] << 8 ) | bytes[1] );
}

int BT2_HasOBEXReply( const struct BTOBEXPush *push );
enum BTHCIResult BT2_ReceiveOBEX( struct BTRFCOMMSession *session, const uint8_t *data, uint32_t length,
	struct BTHCIError *error );
enum BTHCIResult BT2_OBEXRequest( struct BTRFCOMMSession *session, uint8_t *packet, uint32_t length,
	uint8_t expected, struct BTHCIError *error );
enum BTHCIResult BT2_RunObjectPush( struct BTRFCOMMSession *session, struct BTHCIError *error );
enum BTHCIResult BT2_PushObject( const struct BTACLTransport *transport, struct BTOBEXPush *push,
	const uint8_t *key, struct BTHCIError *error );

#endif
