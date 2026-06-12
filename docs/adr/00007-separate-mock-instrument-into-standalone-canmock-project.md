# 00007. Separate mock instrument into standalone CanMock project

2026-06-12

## Status

Accepted

## Context

CanbusMonitor had a `BUILD_SIMULATOR` compile flag that turned it into a CAN transmitter,
sending a fixed table of mock frames to exercise other instruments during development.
Running both roles in one firmware creates coupling: the monitor hardware must be modified
to act as a transmitter, and the simulator code adds complexity to what should be a
read-only diagnostic tool.

Additionally, the Seeed CAN Bus Shield used by CanbusMonitor has a 16 MHz crystal, while
the generic MCP2515 breakout boards typically used for lightweight nodes have an 8 MHz
crystal. These require different library configurations and should not share a codebase.

## Decision

The mock transmitter is extracted into a separate PlatformIO project: `src/CanMock/`.

- Runs on an **Arduino Nano ATmega328P** with a generic MCP2515 breakout board (8 MHz crystal).
- Uses **`coryjfowler/MCP_CAN_lib`**, which accepts the crystal frequency as an explicit
  parameter, making the 8 MHz / 16 MHz distinction explicit in code.
- Has a single purpose: transmit a fixed table of mock CAN frames on a schedule.
- CanbusMonitor is stripped of all `#ifdef BUILD_SIMULATOR` code and the
  `canbusMonitorSimulator` PlatformIO environment. It is listen-only.

## Consequences

- CanbusMonitor firmware is simpler and always listen-only.
- The mock can be flashed to any available Nano without touching the monitor hardware.
- Crystal frequency per hardware type is made explicit at the project level.
- The mock transmit table is maintained in `src/CanMock/` only; no duplication.
