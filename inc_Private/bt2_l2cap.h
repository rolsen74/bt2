#ifndef BT2_L2CAP_H
#define BT2_L2CAP_H

#include "bt2_hci.h"

#define BT_L2CAP_MTU 672
#define BT_L2CAP_HEADER 4
#define BT_L2CAP_SIGNAL_CID 0x0001
#define BT_L2CAP_SIGNAL_HEADER 4
#define BT_L2CAP_SIGNAL_REJECT 0x01

struct BTL2CAPLink
{
	const struct BTACLTransport *Transport;
	struct BTPairRequest Peer;
	uint16_t Handle;
	uint16_t PacketSize;
	uint16_t Credits;
	uint16_t CreditLimit;
	uint16_t PeerMTU;
	const uint32_t *Cancel;
	uint32_t ExtraReadTimeouts;
};

struct BTL2CAPPacket
{
	uint8_t Data[BT_L2CAP_MTU + BT_L2CAP_HEADER];
	uint32_t Length;
	uint16_t CID;
};

struct BTL2CAPSignal
{
	const uint8_t *Data;
	uint16_t Length;
	uint8_t Code;
	uint8_t ID;
};

static inline uint16_t
BT_ReadLE16( const uint8_t *data )
{
	return ( data[0] | ( (uint16_t) data[1] << 8 ) );
}

static inline int
BT2_LinkCancelled( const struct BTL2CAPLink *link )
{
	return ( ( link->Cancel ) && ( __atomic_load_n( link->Cancel, __ATOMIC_RELAXED ) ) );
}

enum BTHCIResult BT2_OpenDataLink( struct BTL2CAPLink *link, const struct BTACLTransport *transport,
	const uint8_t *address, struct BTHCIError *error );
enum BTHCIResult BT2_CloseDataLink( struct BTL2CAPLink *link, enum BTHCIResult previous, struct BTHCIError *error );
enum BTHCIResult BT2_ReadL2CAP( struct BTL2CAPLink *link, struct BTL2CAPPacket *pdu,
	uint32_t *reads, struct BTHCIError *error );
enum BTHCIResult BT2_WriteL2CAP( struct BTL2CAPLink *link, uint16_t cid, const uint8_t *data, uint32_t length,
	struct BTHCIError *error );
enum BTHCIResult BT2_WriteSignal( struct BTL2CAPLink *link, uint8_t code, uint8_t id,
	const uint8_t *data, uint8_t length, struct BTHCIError *error );
int BT2_DecodeSignal( const uint8_t **cursor, uint32_t *remaining, struct BTL2CAPSignal *signal );

enum BTHCIResult BT2_OpenChannel( struct BTL2CAPLink *link, uint16_t psm, uint16_t localCID,
	uint16_t *remoteCID, struct BTHCIError *error );
enum BTHCIResult BT2_ServiceSignal( struct BTL2CAPLink *link, const struct BTL2CAPSignal *signal,
	struct BTHCIError *error );

enum BTHCIResult BT2_OpenDataLinkWithPackets( struct BTL2CAPLink *link, const struct BTACLTransport *transport,
	const uint8_t *address, const uint8_t *key, uint16_t packetTypes, struct BTHCIError *error );
enum BTHCIResult BT2_OpenDataLinkWithKey( struct BTL2CAPLink *link, const struct BTACLTransport *transport,
	const uint8_t *address, const uint8_t *key, struct BTHCIError *error );
enum BTHCIResult BT2_TestRFCOMMWithKey( const struct BTACLTransport *transport, struct BTRFCOMMTest *test,
	const uint8_t *key, struct BTHCIError *error );

enum BTHCIResult BT2_ExchangeRFCOMMWithKey( const struct BTACLTransport *transport, struct BTRFCOMMTransfer *transfer,
	const uint8_t *key, struct BTHCIError *error );

#endif
