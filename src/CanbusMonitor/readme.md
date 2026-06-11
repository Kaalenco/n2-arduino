# Canbus Monitor

Diagnostic tool for CanFIX / CAN bus traffic. Runs on an Arduino Nano ATmega328P with an
MCP2515 CAN controller and a 16×2 character LCD. Useful for verifying that instruments are
transmitting correct data and for diagnosing bus problems during development or installation.

## Hardware

| Component | Notes |
|---|---|
| Arduino Nano ATmega328P | `nanoatmega328` in platformio.ini |
| MCP2515 CAN module (SPI) | CS on D10, SPI on D11/D12/D13 |
| 16×2 LCD with I2C backpack | Default address 0x27 |
| KY-040 rotary encoder with push button | CLK D2, DT D3, BTN D4 |

I2C (A4/A5) is shared between the LCD and any future sensors.

## Display layout

```
Line 1  [STATUS 8 chars][ID count 8 chars]
        ACTIVE    12 IDs
        TIMEOUT   12 IDs    <- bus was active but went quiet for > 3 s
        WAITING...          <- no messages received yet
        BUS ERROR           <- MCP2515 init failed

Line 2  [CAN ID 3 hex] [data bytes]
        0C0 D003 0000       <- RAW_HEX mode: first 4 bytes as hex pairs
        0C0  2500  0512     <- UINT16 mode:  bytes 0-1 and 2-3 as uint16 little-endian
```

Pressing the rotary button cycles between RAW_HEX and UINT16 display modes. Rotating scrolls
through the tracked IDs. Only IDs that have been seen at least once are shown; if no data has
been received yet, line 2 shows "No data".

Status updates are debounced to 250 ms to stay readable.

## Rotary encoder

| Action | Effect |
|---|---|
| Rotate CW / CCW | Select next / previous tracked CAN ID |
| Button press | Toggle display mode (RAW_HEX ↔ UINT16) |

## Serial interface (9600 baud)

Every received CAN frame is echoed to serial as `CAN:0x<id>:<hex bytes>`.

Commands (send with newline):

| Command | Effect |
|---|---|
| `SPEED:125` / `250` / `500` | Set CAN bus speed (kbps); stored in EEPROM, applied on next reset |
| `CLEAR` | Clear the message store; resets tracked IDs and bus-active state |
| `LIST` | Dump all currently tracked IDs and their most recent data bytes |

## EEPROM configuration

EEPROM layout starting at 0x0100 (16 bytes reserved):

| Address | Type | Description |
|---|---|---|
| 0x0100 | uint8_t | Magic byte (0xCB); if absent, defaults are written on boot |
| 0x0101 | uint8_t | CAN speed index: 0 = 125 kbps, 1 = 250 kbps, 2 = 500 kbps (default) |

## Message store

Up to 12 unique CAN IDs are tracked simultaneously (RAM limited on ATmega328P). When the
limit is reached, new IDs are silently dropped. Use `CLEAR` to reset and start fresh.

## Simulator mode

When built with `BUILD_SIMULATOR`, the CAN bus is initialised in normal (transmit) mode
instead of listen-only. A fixed table of `(CAN ID, interval ms, uint16 value)` entries is
transmitted on schedule, and the values are also fed into the local message store so the
display behaves as if they were received externally.

This mode is used to verify that other instruments on the bus correctly interpret the frames
without needing a live engine or sensor source. Build using the `canbusMonitorSimulator`
PlatformIO environment.

Future: add `SIMULATOR_USE_ANALOG` build flag to read A0-A3 as dynamic data sources instead
of the fixed table.

## Build environments

| Environment | Purpose |
|---|---|
| `canbusMonitor` | Normal operation — listen-only on the CAN bus |
| `canbusMonitorSimulator` | Simulator — transmits test frames from a fixed table |
