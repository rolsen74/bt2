#ifndef BT2_ALL_H
#define BT2_ALL_H

#include <proto/exec.h>
#include <dos/dos.h>
#include <devices/newstyle.h>
#include <exec/resident.h>
#include <utility/tagitem.h>
#include "interfaces/bluetooth2.h"
#include "interfaces/bluetooth2sharecontrol.h"

typedef uint16_t U16;
typedef uint32_t U32;
typedef int32_t	 S32;
typedef void *	 PTR;

#define VERSTR				  "v1.34"
#define BLUETOOTH2_VERSION	  1
#define BLUETOOTH2_REVISION	  34
#define BLUETOOTH2_ID		  BLUETOOTH2_NAME " 1.34 (26.11.2026)"

#define BT2_CAP_INQUIRY		  ( 1U << 0 )
#define BT2_CAP_PAIRING		  ( 1U << 1 )
#define BT2_CAP_BONDING		  ( 1U << 2 )
#define BT2_CAP_NAMES		  ( 1U << 3 )
#define BT2_CAP_VISIBILITY	  ( 1U << 4 )
#define BT2_CAP_L2CAP		  ( 1U << 5 )
#define BT2_CAP_SDP			  ( 1U << 6 )
#define BT2_CAP_RFCOMM		  ( 1U << 7 )
#define BT2_CAP_SERIAL		  ( 1U << 8 )
#define BT2_CAP_SERIALFILE	  ( 1U << 9 )
#define BT2_CAP_SERIALCAPTURE ( 1U << 10 )
#define BT2_CAP_OBJECTPUSH	  ( 1U << 11 )
#define BT2_CAP_LE			  ( 1U << 12 )
#define BT2_CAP_A2DP		  ( 1U << 13 )
#define BT2_CAP_PCM			  ( 1U << 14 )
#define BT2_CAP_HID			  ( 1U << 15 )
#define BT2_CAP_HID_INPUT	  ( 1U << 16 )
#define BT2_CAP_SHARE		  ( 1U << 17 )

struct BTController
{
	struct BTController *		Next;
	struct Bluetooth2Base *		Base;
	struct Task *				PrefsWorker;
	uint8_t						Address[6];
	uint8_t						Peer[6];
	uint8_t						Ready;
	uint8_t						Paused;
	uint8_t						ConnectRequested;
	uint8_t						InputAutomatic;
	U32							InputRevision;
	U32							InputState;
	U32							LastResult;
	struct MsgPort *			Port;
	struct Task *				Worker;
	struct BTControllerProbe *	Exclusive;
	struct BTControllerLocation Location;
	S32							Capabilities;
	U32							Unit;
	U32							References;
	U32							Pending;
	U32							Registration;
	uint8_t						PrefsStarting;
};

struct Bluetooth2Base
{
	struct Device			 Device;
	struct ExecIFace *		 Exec;
	struct Unit				 Unit;
	struct SignalSemaphore	 ControllerLock;
	struct SignalSemaphore	 BondLock;
	struct BTController *	 Controllers;
	struct MsgPort *		 MasterPort;
	struct MsgPort *		 CompletionPort;
	struct MsgPort *		 PolicyPort;
	U32						 NextRegistration;
	struct BT2ManagedJob *	 Jobs;
	struct BT2ManagedJob *	 Results;
	U32						 NextJob;
	U32						 ActiveJobs;
	U32						 PolicyRevision;
	uint8_t					 ShareSuppressed;
	uint8_t					 ShareStartupArmed;
	uint8_t					 ShareStartupLoaded;
	uint8_t					 ShareAutomatic;
	U32						 ShareAttemptRevision;
	U32						 ShareStartupState;
	U32						 PlaybackJob;
	struct BT2StoppedInput * StoppedInputs;
	struct BTController *	 ShareController;
	struct Task *			 ShareTask;
	struct BTFileShare		 Share;
	enum BTShareState		 ShareState;
	enum BTHCIResult		 ShareResult;
	struct BTHCIError		 ShareError;
	PTR						 SegList;
};

enum BT2MasterOperation
{
	BT2_MASTER_ADD,
	BT2_MASTER_REMOVE,
	BT2_MASTER_QUEUE,
	BT2_MASTER_CONTROL,
};

struct BT2MasterRequest
{
	struct Message						Message;
	struct MsgPort *					Port;
	const struct BTControllerLocation * Location;
	enum BT2MasterOperation				Operation;
	S32									Capabilities;
	U32									Unit;
	S32									Result;
	const struct BTController *			Expected;
	struct BTController *				Controller;
	struct BTControllerProbe *			Probe;
	struct BTControllerLocation *		Route;
	struct Task *						Caller;
	struct BT2ControlCall *				Control;
};

S32	 BT2_MasterStart ( struct Bluetooth2Base * base );
S32	 BT2_MasterCall ( struct Bluetooth2Base * base, struct BT2MasterRequest * request );
S32	 BT2_AddController ( struct Bluetooth2Base * base, struct MsgPort * port, const struct BTControllerLocation * location,
						 S32 capabilities );
void BT2_RemoveController ( struct Bluetooth2Base * base, U32 unit, struct MsgPort * port );

enum BTHCIResult BT2_QueueProbe ( struct Bluetooth2Base * base, struct BT2MasterRequest * request );
void BT2_RetireProbe ( struct Bluetooth2Base * base, struct BTController * controller, struct BTControllerProbe * probe );
S32	 BT2_CancelQueued ( struct Bluetooth2Base * base, struct BTController * controller, struct BTControllerProbe * probe );
void BT2_StartPolicy ( struct Bluetooth2Base * base );
S32	 BT2_ForwardControl ( struct Bluetooth2Base * base, struct BT2MasterRequest * request );
void BT2_ControlDispatch ( struct Bluetooth2Base * base, struct BT2ControlCall * call );
void BT2_ControlComplete ( struct Bluetooth2Base * base, struct Message * message );
void BT2_ReleaseController ( struct Bluetooth2Base * base, struct BTController * controller );
enum BTHCIResult BT2_RequestForController ( struct Bluetooth2Base * base, U32 unit, const struct BTController * expected,
											struct BTControllerLocation * location, struct BTControllerProbe * probe );
enum BTHCIResult BT2_RequestController ( struct Bluetooth2Base * base, U32 unit, struct BTControllerLocation * location,
										 struct BTControllerProbe * probe );

enum BTHCIResult BT2_Bond ( struct Bluetooth2Base * base, const struct BTHCITransport * transport, struct BTBondRequest * bond,
							struct BTHCIError * error );

enum BTHCIResult BT2_LoadBondKey ( struct Bluetooth2Base * base, const struct BTHCITransport * transport, const uint8_t * peer,
								   uint8_t * key, struct BTHCIError * error );

#endif
