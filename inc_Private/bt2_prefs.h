#ifndef BT2_PREFS_PRIVATE_H
#define BT2_PREFS_PRIVATE_H

#include "bt2_all.h"
#include <interfaces/bluetooth2prefs.h>

struct DOSIFace;

#define BT_BOND_PREFIX "Bluetooth2/"
#define BT_BOND_NAME_SIZE ( sizeof( BT_BOND_PREFIX ) + 25 )
#define BT_BOND_FLAGS ( GVF_GLOBAL_ONLY | GVF_BINARY_VAR )

U32 _main_prefs_Adapters( struct Bluetooth2IFace *self, struct BT2PrefsAdapter *entries, U32 capacity );
enum BTHCIResult _main_prefs_Peers( struct Bluetooth2IFace *self, const uint8_t *local,
	struct BT2PrefsPeer *entries, U32 capacity, U32 *count );
enum BTHCIResult _main_prefs_Action( struct Bluetooth2IFace *self, const uint8_t *local,
	const uint8_t *peer, enum BT2PrefsAction action );

struct BTController *BT2_PrefsFindAdapter( struct Bluetooth2Base *base, const uint8_t *local );
enum BTHCIResult _main_prefs_PeerRequest( struct Bluetooth2IFace *self, const uint8_t *local,
	struct BTInquiryResult *inquiry, struct BTBondRequest *bond, struct BTHCIError *error );

void BT2_ShareRuleName( STRPTR name, const uint8_t *local, const uint8_t *peer );
#define BT_SHARE_RULE_NAME_SIZE ( BT_BOND_NAME_SIZE + 6 )

void BT2_BondName( STRPTR name, const uint8_t *local, const uint8_t *peer );
S32 BT2_ReadAutoPeer( struct DOSIFace *dos, const uint8_t *local, uint8_t *peer );
void BT2_StartPrefsWorker( struct Bluetooth2Base *base, U32 unit, struct MsgPort *port );
void BT2_PrefsInputState( struct BTController *controller, enum BT2PrefsState state );
enum BTHCIResult BT2_PrefsRunInput( struct BTController *controller, const uint8_t *peer );

#endif
