#ifndef BT2_HID_H
#define BT2_HID_H

#include "bt2_l2cap.h"

enum BTHCIResult BT2_MonitorHID( const struct BTACLTransport *transport, struct BTHIDMonitor *monitor,
	const uint8_t *key, int (*expired)( void *context ), void *context, struct BTHCIError *error );

enum BTHCIResult BT2_ConnectHIDInput( const struct BTACLTransport *transport, struct BTHIDInput *input,
	const uint8_t *key, const char **phase, struct BTHCIError *error );
enum BTHCIResult BT2_CheckHIDConnection( struct BTL2CAPLink *link, struct BTHCIError *error );

#endif
