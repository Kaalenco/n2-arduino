# Canbus Monitor

Diagnostic and logging tool for CanFIX / CAN bus traffic. Runs on an Arduino Uno with a
Seeed CAN Bus Shield V2.0 (MCP2515 + SD card). Receives all frames from the bus, displays
them on a 16×2 LCD and stores them to the SD card in CSV format. A connected PC can receive
live frame data over USB serial and manage the device configuration.

## Hardware

| Component | Notes |
|---|---|
| Arduino Uno | `uno` in platformio.ini |
| Seeed CAN Bus Shield V2.0 | MCP2515 CAN (CS D10), SD card (CS D4), SPI shared |
| DS1307 RTC | I2C address 0x68; provides timestamps for log files |
| 16×2 LCD with I2C backpack | Default address 0x27, I2C on A4/A5 |
| Rotary encoder breakout | S1→D5 (CW), S2→D6 (CCW), KEY→D7 (button); active-low, 3-pin module |

I2C (A4/A5) is shared between the LCD, the DS1307 RTC, and any future sensors.

## Display layout

```
Line 1  [STATUS] [ID count] [SD indicator]
        ACT  5 IDs SD       <- active bus, SD card ready
        TMO  5 IDs           <- bus was active but went quiet for > 3 s
        WAITING...  SD       <- no messages yet, SD ready
        BUS ERROR            <- MCP2515 init failed

Line 2  [CAN ID 3 hex] [data bytes]
        0C0 D003 0000        <- RAW_HEX mode: first 4 bytes as hex pairs
        0C0  2500  0512      <- UINT16 mode:  bytes 0-1 and 2-3 as uint16 little-endian
```

Pressing the rotary button cycles between RAW_HEX and UINT16 display modes.
Rotating scrolls through the tracked IDs.

## Rotary encoder

| Action | Effect |
|---|---|
| Rotate CW (S1) | Select next tracked CAN ID |
| Rotate CCW (S2) | Select previous tracked CAN ID |
| Button press (KEY) | Toggle display mode (RAW_HEX ↔ UINT16) |

## Serial interface (115200 baud)

Every received CAN frame is echoed as `CAN:0x<id>:<hex bytes>`.

Every 4 seconds the device sends a status ping:

```
PING can=ACT ids=5 sd=OK rtc=2026-06-12 10:30:45
```

| Field | Values | Meaning |
|---|---|---|
| `can` | `ACT` | CAN frames received within the last 3 s |
| | `TMO` | Bus was active but went quiet (> 3 s) |
| | `WAIT` | No frames received since boot |
| | `ERR` | MCP2515 initialisation failed |
| `ids` | 0–12 | Number of unique CAN IDs tracked in RAM |
| `sd` | `OK` / `FAIL` | SD card state |
| `rtc` | `YYYY-MM-DD HH:MM:SS` | Current time (when RTC is set) |
| | `T+<ms>` | Milliseconds since boot (before `TIME:` is received) |
| | `none` | RTC not set |

Commands (send with newline):

| Command | Effect |
|---|---|
| `TIME:<unix>` | Set RTC clock from Unix timestamp (e.g. `TIME:1749600000`). Saved to DS1307; opens a new dated log file on SD. |
| `DEVID:<hex4>` | Set device ID (e.g. `DEVID:0042`). Stored in EEPROM; used for SD folder and file names. |
| `AIRCRAFT:<hex4>` | Set aircraft identifier (e.g. `AIRCRAFT:PH42`). Stored in EEPROM; prepended to every CSV row. |
| `SPEED:125` / `250` / `500` | Set CAN bus speed (kbps); stored in EEPROM, applied on next reset. |
| `CLEAR` | Clear the message store; resets tracked IDs and bus-active state. |
| `LIST` | Dump all currently tracked IDs and their most recent data bytes. |
| `RESET` | Software-reset the Arduino. |
| `DOWNLOAD:<YYYYMMDD>` | Stream the named CSV file to serial (e.g. `DOWNLOAD:20260610`). |

## SD card logging

Log files are stored in a folder named after the device ID:

```
/<DevID_HEX4>/<DevID_HEX4>_YYYYMMDD.csv
/<DevID_HEX4>/<DevID_HEX4>_NODATE.csv   <- used before TIME: is received
/<DevID_HEX4>/startup.log
```

CSV column order: `aircraft_id,timestamp,can_id,len,b0,b1,b2,b3,b4,b5,b6,b7`

- `aircraft_id` is the 4-digit hex aircraft identifier.
- `timestamp` is `YYYY-MM-DD HH:MM:SS` when the RTC is set, or `T+<ms>` before that.
- Each unique CAN ID is written at most once per 500 ms (~2 rows/sec/ID).

When `TIME:` is received a new dated file is opened and logging switches to it.

## EEPROM configuration

EEPROM layout starting at 0x0100 (16 bytes reserved):

| Address | Type | Description |
|---|---|---|
| 0x0100 | uint8_t | Magic byte (0xCB) |
| 0x0101 | uint8_t | CAN speed index: 0=125, 1=250, 2=500 kbps (default 2) |
| 0x0102 | uint16_t LE | Device ID (default 0x0001) |
| 0x0104 | uint16_t LE | Aircraft ID (default 0x0000) |

## Message store

Up to 12 unique CAN IDs are tracked in RAM simultaneously. When the limit is
reached, new IDs are silently dropped. Use `CLEAR` to reset.

## Startup sequence

1. **Loopback self-test.** MCP2515 is initialised in `MODE_LOOPBACK`. Test frame (ID `0x7FF`, data `A5 5A 42 01`) is sent and received back.
2. **Failure path.** If the test fails, `BUS ERROR` is shown on LCD and `busError` is set. Serial, SD, and the rotary encoder still work.
3. **Switch to normal mode.** On success, briefly switched to `MODE_NORMAL`.
4. **SYSTEM_INIT broadcast.** CAN ID `0x7F0`, byte 0 = `0x02` (CanbusMonitor), bytes 1–2 = `0x00 0x00`.
5. **Switch to listen-only.** Controller switches to `MODE_LISTEN_ONLY` for normal operation.

Serial output on clean boot:
```
RTC OK
SD OK
CANBUS_MONITOR_STARTED
CAN OK, speed: 500 kbps
Device ID:   0x1
Aircraft ID: 0x0
```

## Simulator mode

When built with `BUILD_SIMULATOR` the CAN bus stays in `MODE_NORMAL` (transmit). A fixed
table of `(CAN ID, interval ms, uint16 value)` entries is transmitted on schedule and also fed
into the local message store. SD logging is still active in simulator mode.

## Build environments

| Environment | Purpose |
|---|---|
| `canbusMonitor` | Normal operation — listen-only on the CAN bus |
| `canbusMonitorSimulator` | Simulator — transmits test frames from a fixed table |
