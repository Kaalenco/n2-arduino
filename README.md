# n2-arduino

Arduino-based aircraft instrumentation system. Multiple independent instruments share a CAN bus (CanFIX protocol). Each instrument works standalone with its own local display and also broadcasts its measurements over the bus so other systems can use the data.

## Systems

| System | Status | Description |
|--------|--------|-------------|
| TachometerInstrument | Active | Engine RPM from magneto pickup, OLED display, CAN broadcast |
| CanbusMonitor | Active | Diagnostic tool — listens to all CAN traffic, 16×2 LCD |
| EngineMonitor | Planned | Oil pressure, temperatures, battery |
| NavInstruments | Planned | Altitude, airspeed, GPS, gyro |
| DataLogger | Planned | SD card + WiFi logging |

Code lives in `src/`. Retired example code and earlier iterations are in `ExampleCode/`.

## CAN bus startup protocol

Every system that uses the CAN bus follows the same startup sequence. See [ADR 00006](docs/adr/00006-canbus-startup-verification-and-init-broadcast.md) for the rationale.

### 1 — Loopback self-test

On boot the MCP2515 is initialised in loopback mode. A fixed test frame is sent and received back; if the round-trip fails, `busError` is set and the system runs without CAN (local display continues to work).

### 2 — Switch to operating mode

On success the controller switches to its operating mode: `MODE_NORMAL` for instruments that transmit, `MODE_LISTEN_ONLY` for passive monitors.

### 3 — SYSTEM_INIT broadcast

Immediately after joining the bus, every system transmits a single `SYSTEM_INIT` frame:

| Field | Value |
|-------|-------|
| CAN ID | `0x7F0` |
| Byte 0 | System type code |
| Byte 1–2 | Primary data CAN ID (little-endian); `0x0000` for passive listeners |

**System type codes:**

| Code | System |
|------|--------|
| `0x01` | TachometerInstrument |
| `0x02` | CanbusMonitor |
| `0x10` | EngineMonitor |
| `0x20` | NavInstruments |
| `0x30` | DataLogger |

## Architecture decisions

ADRs are in `docs/adr/`. Key decisions:

- [00002](docs/adr/00002-use-of-platformio.md) — PlatformIO over Visual Micro
- [00003](docs/adr/00003-user-interaction.md) — Event-driven UI architecture
- [00006](docs/adr/00006-canbus-startup-verification-and-init-broadcast.md) — CAN startup verification and SYSTEM_INIT broadcast
