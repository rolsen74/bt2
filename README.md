# Bluetooth2

Experimental Bluetooth host stack for AmigaOS 4.

Bluetooth2 probes local controllers and discovers nearby Classic Bluetooth devices.
HCI command framing and event parsing use a transport interface.

| Path | Contents |
| --- | --- |
| `Stack/HCI/` | Controller commands and HCI event parsing |
| `Stack/Device/` | Exec device, cold-start and after-DOS residents, and main interface |
| `Stack/Master/` | After-DOS process and controller registration |
| `inc_Public/` | Host API and transport interface |

## Build

Run `make` with the AmigaOS 4 toolchain to build `bin/bluetooth2.kmod`.
The module registers `bluetooth2.device` at cold start. Its after-DOS
resident starts the `Bluetooth` process and logs
`Bluetooth2 : Full Boot`. The master handles
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
archive `bt2.lha`. The module is packaged at `bt2/Kickstart/bluetooth2.kmod`.

Run `make archive` to build `bin/libbt2.a` for programs that link the HCI
code directly.

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
pairing and discarded. Secure Simple Pairing, BLE, and profiles are pending.

See the [usb2 project](https://github.com/rolsen74/usb2).
