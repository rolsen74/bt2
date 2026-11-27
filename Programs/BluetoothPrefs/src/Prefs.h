#ifndef BLUETOOTH_PREFS_APP_H
#define BLUETOOTH_PREFS_APP_H

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <classes/window.h>
#include <gadgets/layout.h>
#include <gadgets/chooser.h>
#include <gadgets/listbrowser.h>
#include <gadgets/string.h>
#include <interfaces/listbrowser.h>
#include <interfaces/bluetooth2.h>

#define PREFS_ADAPTERS 16
#define PREFS_PEERS 64

enum PrefsClass
{
	CLASS_WINDOW,
	CLASS_LAYOUT,
	CLASS_LIST,
	CLASS_STRING,
	CLASS_BUTTON,
	CLASS_CHOOSER,
	CLASS_COUNT,
};

enum GadgetID
{
	GAD_ADAPTER = 1,
	GAD_DEVICES,
	GAD_PIN,
	GAD_SCAN,
	GAD_PAIR,
	GAD_CONNECT,
	GAD_DISCONNECT,
	GAD_FORGET,
	GAD_AUTO,
	GAD_SHARE_DIR,
	GAD_SHARE_RULE,
	GAD_SHARE_START,
	GAD_SHARE_BOOT,
	GAD_SHARE_STATUS,
	GAD_AUDIO_OUTPUT,
	GAD_AUDIO_PLAY,
	GAD_AUDIO_STOP,
	GAD_AUDIO_STATUS,
	GAD_STATUS,
	GAD_COUNT,
};

struct PeerRow
{
	uint8_t Address[6];
	uint32_t Class;
	uint8_t Saved;
	uint8_t Auto;
};

struct PrefsJob
{
	struct BTInquiryDevice Nearby[PREFS_PEERS];
	uint32_t ID;
	int Pair;
};

struct PrefsWindow
{
	struct Bluetooth2IFace *Bluetooth;
	struct ListBrowserIFace *ListBrowser;
	struct Window *Window;
	struct List Rows;
	struct ColumnInfo *Columns;
	struct MsgPort *AppPort;
	Object *Object;
	Object *Gadgets[GAD_COUNT];
	Class *Classes[CLASS_COUNT];
	struct ClassLibrary *ClassLibraries[CLASS_COUNT];
	struct PrefsJob Job;
	struct BTShareStatus ShareStatus;
	struct BT2StartupShare BootShare;
	enum BT2StartupShareState BootShareState;
	struct BT2AudioOutput AudioOutput;
	uint32_t AudioJob;
	uint32_t Registration;
	char ShareLabel[96];
	char AudioLabel[96];
	struct BT2PrefsAdapter Adapter;
	struct PeerRow Peers[2 * PREFS_PEERS];
	uint32_t PeerCount;
	uint32_t Reports;
	uint8_t Selection[6];
	int HaveSelection;
	int HaveAdapter;
	int Busy;
	int Closing;
	int Sharing;
	char SharePath[BT_SHARE_PATH_SIZE];
	uint32_t AdapterCount;
	char AdapterText[PREFS_ADAPTERS][48];
	const char *AdapterLabels[PREFS_ADAPTERS + 1];
	char PIN[BT_PIN_MAX + 1];
	char PINUndo[BT_PIN_MAX + 1];
	char PINWork[BT_PIN_MAX + 1];
	char Status[100];
};

#define PREFS_VERSION "Bluetooth2 1.8 (26.11.2026)"

int Prefs_OpenWindow( struct PrefsWindow *gui );
void Prefs_CloseWindow( struct PrefsWindow *gui );
void Prefs_RebuildRows( struct PrefsWindow *gui, int scan );
void Prefs_RefreshControls( struct PrefsWindow *gui );
void Prefs_SetStatus( struct PrefsWindow *gui, const char *text );
void Prefs_ResultStatus( struct PrefsWindow *gui, enum BTHCIResult result, const struct BTHCIError *error );
void Prefs_FormatAddress( char *text, const uint8_t *address );
int Prefs_StartJob( struct PrefsWindow *gui, int pair );
void Prefs_PollJob( struct PrefsWindow *gui );
void Prefs_RefreshAudio( struct PrefsWindow *gui );
void Prefs_AudioAction( struct PrefsWindow *gui, ULONG id );
void Prefs_AudioTick( struct PrefsWindow *gui );
void Prefs_RefreshShare( struct PrefsWindow *gui );
void Prefs_ShareAction( struct PrefsWindow *gui, ULONG id );
void Prefs_ShareTick( struct PrefsWindow *gui );
struct PeerRow *Prefs_Selected( struct PrefsWindow *gui );

#endif
