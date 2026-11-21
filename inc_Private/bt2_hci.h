#ifndef BT2_HCI_H
#define BT2_HCI_H

#include "Bluetooth.h"

#define HCI_EVENT_COMMAND_COMPLETE 0x0e
#define HCI_EVENT_COMMAND_STATUS 0x0f
#define HCI_EVENT_HEADER 2
#define HCI_EVENT_MAX ( HCI_EVENT_HEADER + 255 )

enum BTHCIResult BT2_ReadEvent( const struct BTHCITransport *transport, uint8_t *event,
	uint32_t *length, uint32_t *reads, struct BTHCIError *error );

#define BT_LINK_KEY_SIZE 16

struct BTPairKeys
{
	const uint8_t *Saved;
	uint8_t New[BT_LINK_KEY_SIZE];
	uint8_t NewValid;
	uint8_t Sent;
};

enum BTHCIResult BT2_PairWithKeys( const struct BTHCITransport *transport,
	const struct BTPairRequest *pair, struct BTPairKeys *keys, struct BTHCIError *error );

#endif
