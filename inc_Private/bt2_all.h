#ifndef BT2_ALL_H
#define BT2_ALL_H

#include <proto/exec.h>
#include <dos/dos.h>
#include <devices/newstyle.h>
#include <exec/resident.h>
#include <utility/tagitem.h>
#include "interfaces/bluetooth2.h"

typedef uint16_t U16;
typedef uint32_t U32;
typedef int32_t S32;
typedef void *PTR;

#define BLUETOOTH2_VERSION 1
#define BLUETOOTH2_REVISION 5
#define BLUETOOTH2_ID BLUETOOTH2_NAME " 1.5 (21.11.2026)"

#define BT2_CAP_INQUIRY ( 1U << 0 )
#define BT2_CAP_PAIRING ( 1U << 1 )
#define BT2_CAP_BONDING ( 1U << 2 )

struct BTController
{
	struct BTController *Next;
	struct MsgPort *Port;
	struct BTControllerLocation Location;
	S32 Capabilities;
	U32 Unit;
};

struct Bluetooth2Base
{
	struct Device Device;
	struct ExecIFace *Exec;
	struct Unit Unit;
	struct SignalSemaphore ControllerLock;
	struct SignalSemaphore BondLock;
	struct BTController *Controllers;
	struct MsgPort *MasterPort;
	PTR SegList;
};

enum BT2MasterOperation
{
	BT2_MASTER_ADD,
	BT2_MASTER_REMOVE,
};

struct BT2MasterRequest
{
	struct Message Message;
	struct MsgPort *Port;
	const struct BTControllerLocation *Location;
	enum BT2MasterOperation Operation;
	S32 Capabilities;
	U32 Unit;
	S32 Result;
};

S32 BT2_MasterStart( struct Bluetooth2Base *base );
S32 BT2_MasterCall( struct Bluetooth2Base *base, struct BT2MasterRequest *request );
S32 BT2_AddController( struct Bluetooth2Base *base, struct MsgPort *port,
	const struct BTControllerLocation *location, S32 capabilities );
void BT2_RemoveController( struct Bluetooth2Base *base, U32 unit, struct MsgPort *port );

enum BTHCIResult BT2_RequestController( struct Bluetooth2Base *base, U32 unit,
	struct BTControllerLocation *location, struct BTControllerProbe *probe );

enum BTHCIResult BT2_Bond( struct Bluetooth2Base *base, const struct BTHCITransport *transport,
	struct BTBondRequest *bond, struct BTHCIError *error );

#endif
