# Bluetooth2

Experimental Bluetooth host stack for AmigaOS 4.

Bluetooth2 probes local controllers and discovers nearby Classic Bluetooth devices.
It also scans and advertises on Bluetooth LE controllers. HCI command framing
and event parsing use a transport interface.

| Path | Contents |
| --- | --- |
| `Stack/HCI/` | Controller commands and HCI event parsing |
| `Stack/L2CAP/` | Classic data channels and signaling |
| `Stack/SDP/` | Public service discovery |
| `Stack/RFCOMM/` | Serial channel setup and data exchange |
| `Stack/OBEX/` | Outgoing Bluetooth Object Push over RFCOMM |
| `Stack/A2DP/` | AVDTP signaling and SBC playback to Classic audio sinks |
| `Stack/HID/` | Classic Bluetooth HID Boot Protocol report monitoring and live input |
| `Stack/Device/` | Exec device, cold-start and after-DOS residents, and main interface |
| `Stack/Master/` | After-DOS process and controller registration |
| `inc_Public/` | Host API and transport interface |

## Build

Run `make` with the AmigaOS 4 toolchain to build `bin/bluetooth2.kmod`.
The module registers `bluetooth2.device` at cold start. Its after-DOS
resident starts the `Bluetooth` process and logs
`Bluetooth : Full Boot`. The master handles
controller registration and removal. Each transport task runs its own I/O.
Open unit 0 with `OpenDevice()`, then obtain the version 4 `main`
interface declared in `inc_Public/interfaces/bluetooth2.h`. It provides
controller registration, discovery, pairing, transport operations, audio, HID,
preferences, and file sharing. Exec uses the `__device` manager interface
for device lifecycle and I/O requests such as `NSCMD_DEVICEQUERY`.
Install the rebuilt Bluetooth USB driver and preferences app with this module
so they use the same interface version.

To try the module at boot, copy it to `SYS:Kickstart/` and add the following
line to the desired Kicklayout entry.

```text
MODULE Kickstart/bluetooth2.kmod
```

With `lha` installed, run `make -f makefile_archive` to build the distribution
archive `bt2.lha`. The module is packaged at `bt2/Kickstart/bluetooth2.kmod`
along with the required license notice for [libsbc](ThirdParty/libsbc/README.md).

Run `make archive` to build `bin/libbt2.a` for programs that link the HCI,
L2CAP, SDP, RFCOMM, OBEX, or A2DP code directly.

Install `Bluetooth.Class` and `Bluetooth.Driver` from the sibling
`usb2` repository in `LIBS:USB2/`. The USB driver adds a controller when
it binds and removes it on detach. Controller numbers stay stable while
the adapters are attached and freed numbers can be assigned again.
`USB2BluetoothTest` uses this registry to probe a controller by unit.
With Bluetooth.Driver 1.1, run `USB2BluetoothTest DISCOVER` for Classic
Bluetooth discovery on unit 0 or add `UNIT=1` for the second adapter.
Bluetooth2 1.4 introduced Classic pairing. Use
`USB2BluetoothTest PAIR=AA:BB:CC:DD:EE:FF` with Bluetooth.Driver 1.2
and a Classic device that supports legacy PIN pairing. The test opens an
ACL link, verifies authentication and encryption, and disconnects.
Bluetooth2 1.5 adds `BOND=<address>` and `RECONNECT=<address>` in
USB2BluetoothTest with Bluetooth.Driver 1.3. The stack saves the bond only
when requested, using one binary key per local adapter and peer address
under `ENV:Bluetooth2/` and `ENVARC:Bluetooth2/`. The PIN is used during
pairing and discarded. Bluetooth2 1.6 adds `NAME=<address>` to look up
a Classic Bluetooth device's name with Bluetooth.Driver 1.4.
Bluetooth2 1.7 and Bluetooth.Driver 1.5 let a nearby device discover the
Amiga using `USB2BluetoothTest VISIBLE AS="Amiga" SECONDS=30`.
Bluetooth2 1.8 and Bluetooth.Driver 1.6 add `USB2BluetoothTest ECHO=<address>`
to test a Classic ACL data link with an L2CAP echo request.
Bluetooth2 1.9 and Bluetooth.Driver 1.7 add `USB2BluetoothTest SERVICES=<address>`
to list public Classic Bluetooth SDP records, with service-class UUIDs and names.
Bluetooth2 1.10 and Bluetooth.Driver 1.8 show published RFCOMM channels
in the service list. Use `USB2BluetoothTest RFCOMM=<address> CHANNEL=1`
with the peer's channel to exercise RFCOMM setup and teardown. A saved
bond is used for authentication and encryption when available.
Bluetooth2 1.11 and Bluetooth.Driver 1.9 add `TEXT="Hello" REPLY` to send
serial text and display the first reply frame. Bluetooth2 1.12 and
Bluetooth.Driver 1.10 add `FILE=<path>` to stream a binary file to a serial
receiver using RFCOMM credit-based flow control. Bluetooth2 1.13 and
Bluetooth.Driver 1.11 add `SAVE=<path> BYTES=<count>` to capture an exact
byte count to a new file. Ctrl-C cancels either transfer.
Bluetooth2 1.14 and Bluetooth.Driver 1.12 add `PUSH=<path>` for sending
a file to a Bluetooth Object Push receiver. Bluetooth2 1.16 looks up the
receiver's RFCOMM channel directly by service class `$1105` when
`CHANNEL` is omitted. USB2BluetoothTest 0.18 adds a ReAction window
for Classic scanning and outgoing file transfers with `GUI`.
Bluetooth2 1.15 and Bluetooth.Driver 1.13 add `LESCAN` and
`LEADVERTISE AS="Amiga"` for controllers that support legacy LE
advertising. `PASSIVE` makes the scan listen without sending scan
requests. Both commands take `SECONDS` and can be stopped with Ctrl-C.
Bluetooth2 1.17, Bluetooth.Driver 1.14, and USB2BluetoothTest 0.19 add
experimental A2DP Source playback. Use `SPEAKER=<address> PLAY=<path>`
or choose **Play WAV** in the GUI. Playback accepts 16-bit mono or stereo
PCM WAV files at 44.1 or 48 kHz. Bluetooth2 1.18 adds a live stereo PCM
interface for the Bluetooth2 AHI driver in the sibling USB2 repository.
Bluetooth2 1.19 groups SBC frames to reduce USB transfers and lowers
the encoded bitrate when the stream repeatedly runs late. Bluetooth.Driver
1.16 queues the HCI event read ahead of audio writes. The serial timing
summary records audio duration, send time, and late packets.
Bluetooth2 1.20 allows the controller to use higher-throughput BR/EDR
packet types on A2DP connections. The revised playback timing is awaiting
a hardware test.
Bluetooth2 1.21 and Bluetooth.Driver 1.17 add
`USB2BluetoothTest HID=<address> SECONDS=15` for Classic HID Boot Protocol
keyboards and mice. The test opens the HID control and interrupt channels
and reports the received input in the Shell. Bluetooth2 1.22,
Bluetooth.Driver 1.18, and USB2BluetoothTest 0.22 add `HID=<address> INPUT`
to route a bonded device to AmigaOS `input.device`. Hardware testing and
HID over GATT are pending.
An Amiga serial device, Secure Simple Pairing, LE connections, and GATT
are pending.

See the [usb2 project](https://github.com/rolsen74/usb2).


## Bluetooth Preferences

Install `Kickstart/bluetooth2.kmod`, `Prefs/Bluetooth2`, and `Prefs/Bluetooth2.info` from `bt2.lha`. USB adapters also need the Bluetooth class driver from `usb2.lha`.

Open Bluetooth in Prefs, scan for Classic devices, select a mouse or keyboard, and enter the device PIN to pair it. Connect routes input to AmigaOS. Auto-connect saves one preferred input device per adapter and keeps reconnecting after Prefs closes and across boots. Disconnect pauses reconnect for the current boot, including if the same adapter is unplugged and reattached. Forget deletes the saved bond.

Bluetooth Full Boot applies saved settings as adapters become ready. Prefs can scan, pair, or connect using a second adapter while another adapter is busy. An active Classic HID, WAV, or file-sharing session reserves its adapter. Automatic reconnect uses host-initiated Classic HID. Bluetooth LE HID and Secure Simple Pairing are outside the current stack's support.

### Bluetooth file sharing

In **Prefs/Bluetooth2**, choose a **Shared folder**, select a paired device, and set its file access. Access defaults to **No access**. **Read only** allows listing and downloading files. **Write only** allows uploading new files. **Read and write** enables both.

Select **Start sharing** and connect from the paired device with a Classic Bluetooth OBEX File Transfer (FTP) client over RFCOMM. The share exposes files in the selected folder. Existing files are preserved when an upload uses the same name. Sharing continues if you close or iconify Prefs. Reopen Prefs and select **Stop sharing** to end the listener. The listener uses the selected adapter while active.


Choose **Enable at boot** with a folder and adapter selected to save a startup share. At most one automatic service can use a stick, so a stick configured for input auto-connect cannot also start sharing at boot. **Start sharing** starts it now. **Stop sharing** pauses automatic sharing for the current boot, including across replug.

### Bluetooth audio

Pair a Classic Bluetooth speaker or headset and choose **Use selected device** in the audio group. **Play WAV** uses that saved output and **Stop** ends the current playback. Use another stick for input or file sharing during playback.
