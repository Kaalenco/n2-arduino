# Project setup

This project will contain code for engine monitor instruments, aircraft instruments and montoring systems.
Monitors and intruments have their own specific hardware. For example: Engine instruments are linked to
one or more Arduino board, each having their own display (I2C) and a Canbus adapter. The canbus is
used to send information to a common monitor (cockpit display). I2C display is used to show the value(s) 
directly. With this setup, each instrument can function as a seperate unit and in combination with others.

## Instruments

### Temperature based (Max6675)

CHT - Cylinder head temperature
EGT - Exhaust gas temperature
AMB - Ambient (outside) temperature
EOT - Engine Oil temperature

### Analog based

EOP - Engine oil pressure
ADP - Atmospheric dynamic pressure

### Others

ASP - Atmospheric static pressure

### Derived

ALT - Altitude
IAS - Indicated Air Speed

### Pulse based

RPM - Engine RPM

> A single Arduino can connect to more than one Max6675, using different ports for CS.

## Canbus

The canbus standard defines how data is transmitted. CanFix is used to define
the instrument identification and values. See [Canfix Folder](../docs/canfix)

## CAN bus startup protocol

Every CAN-capable system follows the same startup sequence on boot.

### Loopback self-test

The MCP2515 is first initialised in loopback mode. A fixed test frame (CAN ID `0x7FF`, data `A5 5A 42 01`) is sent and received back. If the round-trip fails the system sets `busError = true`, logs the failure to serial, and continues without CAN. The local display shows `BUS ERROR`.

### SYSTEM_INIT broadcast

If the self-test passes, the system switches to its operating mode and immediately broadcasts a single `SYSTEM_INIT` frame (CAN ID `0x7F0`):

| Byte | Content |
|------|---------|
| 0 | System type code |
| 1–2 | Primary data CAN ID, little-endian (`0x0000` for passive listeners) |

**System type codes:**

| Code | System |
|------|--------|
| `0x01` | TachometerInstrument |
| `0x02` | CanbusMonitor |
| `0x10` | EngineMonitor |
| `0x20` | NavInstruments |
| `0x30` | DataLogger |

Systems that listen in `MODE_LISTEN_ONLY` (CanbusMonitor) switch briefly to `MODE_NORMAL` to send the init frame, then switch back.

See [ADR 00006](../docs/adr/00006-canbus-startup-verification-and-init-broadcast.md) for the full rationale.