# 00006. CAN bus startup verification and system initialized broadcast

2026-06-11

## Status

__New__

## Context

The system consists of multiple independent Arduino-based instruments communicating over a shared CAN bus. Instruments are designed to function standalone, but rely on CAN being operational to integrate with the rest of the network.

Two problems arise without an explicit startup protocol:

1. **Silent hardware failure.** If the MCP2515 CAN controller fails to initialise (bad SPI wiring, damaged chip, absent power), the firmware continues running without CAN and there is no indication that measurements are not being broadcast. This is dangerous in an aircraft instrumentation context.

2. **No presence detection.** When a CAN bus monitor or data logger starts up, it cannot know which instruments are already active on the bus. Instruments only transmit their measurement frames, so there is no way to distinguish "instrument running but engine stopped (RPM = 0)" from "instrument absent".

The `test_CanBus` unit test in ExampleCode demonstrates that the MCP2515 can be verified in loopback mode by sending a frame and receiving it back. The same technique is applicable at runtime during startup.

## Decision

Every system that uses the CAN bus SHALL follow this startup sequence:

### 1. Loopback self-test

Initialise the MCP2515 in `MODE_LOOPBACK`. Send a fixed test frame, wait 10 ms, and receive it back. Verify that the received CAN ID and all data bytes match.

Test frame:
| Field | Value |
|-------|-------|
| CAN ID | `0x7FF` |
| Length | 4 bytes |
| Data | `A5 5A 42 01` |

This matches the pattern used in the `test_loopback_send_receive` test in ExampleCode.

### 2. Failure handling

If the self-test fails: log the error to serial, set `busError = true`, and skip all further CAN operations. The system continues operating locally (local display and sensors work; CAN does not). The local display SHALL show `BUS ERROR` or equivalent.

### 3. Switch to operating mode

If the self-test passes, call `setMode()` to transition to the normal operating mode:
- `MODE_NORMAL` — systems that transmit measurements
- `MODE_LISTEN_ONLY` — passive monitors (CanbusMonitor)

### 4. Broadcast SYSTEM_INIT

Immediately after switching to operating mode, transmit a single `SYSTEM_INIT` frame:

| Field | Value |
|-------|-------|
| CAN ID | `0x7F0` |
| Length | 3 bytes |
| Byte 0 | System type code (see table) |
| Byte 1–2 | Primary data CAN ID, little-endian (`0x0000` for passive listeners) |

Systems that operate in `MODE_LISTEN_ONLY` MUST switch briefly to `MODE_NORMAL` to transmit the `SYSTEM_INIT` frame, then switch back to `MODE_LISTEN_ONLY`.

### System type codes

| Code | System |
|------|--------|
| `0x01` | TachometerInstrument |
| `0x02` | CanbusMonitor |
| `0x10` | EngineMonitor (planned) |
| `0x20` | NavInstruments (planned) |
| `0x30` | DataLogger (planned) |

New systems must be assigned a unique code; this table is the authoritative mapping.

## Consequences

- CAN hardware faults are detected and reported at startup rather than silently producing missing data.
- Diagnostic tools (CanbusMonitor) can log which instruments came online and when.
- Startup time increases by approximately 10–20 ms (loopback delay + mode switch).
- Passive monitors (CanbusMonitor) need a brief switch to `MODE_NORMAL` to transmit the init frame; this is acceptable given the short duration.
- Every new CAN-capable system must register a system type code in this ADR.
