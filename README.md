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
Open unit 0 with `OpenDevice()`, then obtain the version 3 `main`
interface declared in `inc_Public/interfaces/bluetooth2.h`. It exposes
HCI reset, controller information, Classic Bluetooth discovery, and registration
of hot-pluggable controllers. Exec I/O requests support `NSCMD_DEVICEQUERY`.

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
Bluetooth2 1.4 adds the version 1 `pairing` interface. Use
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
