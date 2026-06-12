# CanMock

CAN bus traffic generator for bench testing other modules. Simulates a running engine by sending RPM and temperature data on a fixed schedule.

## Hardware

| Item | Detail |
|------|--------|
| Board | Arduino Nano ATmega328 |
| CAN module | Generic MCP2515 breakout, **8 MHz** crystal |
| CAN CS pin | D10 |
| Serial | 115200 baud, COM5 |

## CAN messages produced

All IDs follow the [CanFIX](../../docs/canfix/src/canfix.json) parameter specification.

| CanFIX ID | Hex | Content | Interval | Notes |
|-----------|-----|---------|----------|-------|
| 512 | `0x200` | RPM (uint16 LE, direct) | 1 s | Cycles through the RPM state machine |
| 1282 | `0x502` | EGT (uint16 LE, ×0.1 °C) | 10 s | Triangle wave 80–180 °C |
| 1280 | `0x500` | CHT (uint16 LE, ×0.1 °C) | 10 s | Triangle wave 70–120 °C |
| — | `0x7F0` | SYSTEM_INIT `[0x10, 0, 0]` | Once on boot | Identifies this node as type Mock (0x10) |

### RPM state machine

The RPM output cycles through three states in a continuous loop:

| State | RPM | Duration |
|-------|-----|----------|
| Idle | 700 | 10 s |
| Cruise | 1700 | 10 s |
| Overspeed | 2700 | 2 s |

### Temperature waveforms

EGT and CHT both produce a triangle wave that rises and falls over 200 steps.
Wire encoding follows CanFIX ×0.1 °C (multiply °C by 10 for the raw uint16 value).

- **EGT**: 800–1800 raw (80.0–180.0 °C)
- **CHT**: 700–1200 raw (70.0–120.0 °C)

## CAN messages consumed

| CAN ID | Action |
|--------|--------|
| `0x7EF` SYSRESET | Resets the MCU via null function pointer |
| `0x7E0` CONFIG | Logs `type`, `param`, `value` to Serial; no side effects |

## Serial output

All events are logged to Serial at 115200 baud. On startup:

```
CAN_MOCK_STARTED
Firmware: v1.0.x
```

During operation, state changes and temperature steps are printed on each interval.

## Build

```bash
cd src/CanMock
C:\Users\gjkaa\.platformio\penv\Scripts\pio run
C:\Users\gjkaa\.platformio\penv\Scripts\pio run --target upload
```

The build number increments automatically on each compile via `increment_build.py`.
