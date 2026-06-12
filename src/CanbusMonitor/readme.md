# CanbusMonitor — v1.0.5

Diagnostic and logging tool for CanFIX / CAN bus traffic. Runs on an Arduino Uno with a
Seeed CAN Bus Shield V2.0 (MCP2515 + SD card). Receives all instrument frames from the bus,
displays them on a 16×2 LCD, stores them to the SD card in CSV format, and provides a serial
interface for configuration and bus commands.

## Hardware

| Component | Detail |
|---|---|
| Arduino Uno | ATmega328P, 16 MHz |
| Seeed CAN Bus Shield V2.0 | MCP2515 CAN controller (CS D10, 16 MHz crystal), SD card (CS D4), SPI shared |
| DS1307 RTC | I2C address 0x68; provides timestamps for SD log files |
| 16×2 LCD with I2C backpack | PCF8574T, default address 0x27, I2C on A4/A5 |
| KY-040 rotary encoder | CLK→D5, DT→D6, SW→D7; active-low with internal pull-ups |

SPI bus (D10–D13) is shared between the MCP2515 CAN controller and the SD card.
I2C bus (A4/A5) is shared between the LCD and the DS1307 RTC.

## Pin assignments

| Pin | Function |
|---|---|
| D4 | SD card CS |
| D5 | Rotary encoder CLK (S1) |
| D6 | Rotary encoder DT (S2) |
| D7 | Rotary encoder push button (KEY) |
| D10 | MCP2515 CAN controller CS (fixed) |
| D11 | SPI MOSI (fixed) |
| D12 | SPI MISO (fixed) |
| D13 | SPI SCK (fixed) |
| A4 | I2C SDA (fixed) |
| A5 | I2C SCL (fixed) |

## Display layout

```
┌────────────────┐
│ACT  5 IDs SD  \│  ← line 1: status, ID count, SD indicator, spinner
│RPM   2450      │  ← line 2: selected instrument value
└────────────────┘
```

### Line 1 — status

| Prefix | Meaning |
|---|---|
| `ACT` | CAN frames received within the last 3 s |
| `TMO` | Bus was active but went quiet (> 3 s) |
| `WAITING...` | No frames received since boot |
| `BUS ERROR` | MCP2515 initialisation failed |

`SD` appears when the SD card is mounted. A spinner character (\\|/−) animates at the
far right on every 250 ms refresh to confirm the firmware is running.

### Line 2 — instrument value

The rotary encoder scrolls through all CAN IDs currently in the store.

For **known instrument IDs**, the format is:

```
Col:  0123456789012345
      VVV nnnnnn UUU**
      RPM   2450      (no warning, no unit)
      EGT    145 °C   (no warning)
      EGT    145 °C HI (caution high exceeded)
      CHT    107 °C LO (caution low exceeded)
```

- Columns 0–2: 3-char mnemonic, left-aligned.
- Columns 4–9: numeric value, **right-aligned** in a 6-char field.
- Columns 10–12: unit, left-aligned in a 3-char field. Omitted for RPM.
- Columns 13–15: `HI` / `LO` warn flag (right-aligned), spaces when no alert.
- Temperature values are stored ×10 (0.1 °C resolution) and displayed as integer °C.
- The `°` symbol is a custom CGRAM character (slot 1) defined at startup.

For **unknown CAN IDs**, raw bytes are shown:

```
???  A5 5A 42 01
```

### Known instrument IDs

| CAN ID | Mnemonic | Unit | Scale | Default warn HI |
|--------|----------|------|-------|-----------------|
| `0x0C0` | `RPM` | — (not shown) | ×1 | 2800 |
| `0x0D0` | `EGT` | °C | ×10 (0.1 °C) | 1500 raw (150 °C) |
| `0x0D1` | `CHT` | °C | ×10 (0.1 °C) | 1100 raw (110 °C) |

Warning thresholds are RAM-resident and reset on power cycle. They can be updated via
`SET:` serial commands (see below) or by the EMS-App.

CAN IDs ≥ `0x7E0` (system and command frames) are never stored or displayed.

## Rotary encoder

| Action | Effect |
|---|---|
| Rotate CW | Select next tracked CAN ID |
| Rotate CCW | Select previous tracked CAN ID |
| Short press | Toggle LCD backlight on/off |
| Hold ≥ 5 s | Software reset |

Direction decoding uses the KY-040 quadrature method: direction is sampled on the rising
edge of CLK (S1). Debounce: 50 ms cooldown on CLK, 20 ms on the push button.

## Startup sequence

On power-on, the LCD shows `Canbus Monitor` / `v<MAJOR>.<MINOR>.<BUILD>` while hardware
initialises.

1. **Loopback self-test.** MCP2515 is put in `MODE_LOOPBACK`. Test frame (ID `0x7FF`,
   data `A5 5A 42 01`) is sent and echoed back internally.
2. **Failure path.** If the test fails, `BUS ERROR` is shown. Serial, SD, and the rotary
   encoder still operate.
3. **Normal mode.** On success, MCP2515 switches to `MODE_NORMAL`.
4. **SYSTEM_INIT broadcast.** CAN ID `0x7F0`, byte 0 = `0x02` (CanbusMonitor type code).
5. **Re-init.** Controller re-initialised to clear any Bus-Off state from the unanswered
   broadcast, then switches to `MODE_LISTEN_ONLY` for normal operation.

Serial output on a clean boot:

```
RTC OK
SD OK
CANBUS_MONITOR_STARTED
Firmware: v1.0.5
CAN OK, speed: 500 kbps
Device ID:   0x1
Aircraft ID: 0x0
```

## Serial interface (115200 baud)

Every received instrument CAN frame is echoed as:
```
CAN:0x<id>:<hex bytes>
```

Every 4 seconds a status ping is sent:
```
PING can=ACT ids=5 sd=OK rtc=2026-06-12 10:30:45
```

| Field | Values | Meaning |
|---|---|---|
| `can` | `ACT` / `TMO` / `WAIT` / `ERR` | Bus activity state |
| `ids` | 0–12 | Unique CAN IDs in the store |
| `sd` | `OK` / `FAIL` | SD card state |
| `rtc` | `YYYY-MM-DD HH:MM:SS` | RTC time (when set) |
| | `T+<ms>` | Milliseconds since boot (before `TIME:` received) |
| | `none` | RTC absent or unset |

### Configuration commands

| Command | Effect |
|---|---|
| `TIME:<unix>` | Set RTC from Unix timestamp (e.g. `TIME:1749600000`). Saved to DS1307; opens a new dated log file on SD. |
| `DEVID:<hex4>` | Set device ID (e.g. `DEVID:0042`). Non-zero, stored in EEPROM. |
| `AIRCRAFT:<hex4>` | Set aircraft ID (e.g. `AIRCRAFT:PH42`). Stored in EEPROM. |
| `SPEED:125` / `250` / `500` | Set CAN bus speed in kbps. Stored in EEPROM; applied on next reset. |
| `CLEAR` | Clear the message store and reset bus-active state. |
| `LIST` | Dump all tracked IDs and their most recent data bytes to serial. |
| `RESET` | Software-reset the Arduino. |
| `DOWNLOAD:<YYYYMMDD>` | Stream the named CSV log file to serial (e.g. `DOWNLOAD:20260612`). |

### Bus command and configuration commands

| Command | Effect |
|---|---|
| `SYSRESET` | Broadcast CAN `0x7EF` (zero bytes). All nodes reset. |
| `SET:<TYPE>:<PARAM>:<VALUE>` | Send CAN CONFIG frame `0x7E0` to the target instrument type with the given parameter and value. Also updates the local warning threshold if applicable. |

`SET` examples:

```
SET:RPM:GRL:700      # RPM green arc low
SET:RPM:GRH:2500     # RPM green arc high
SET:RPM:RED:2800     # RPM redline
SET:ALT:QNH:1013     # Altimeter QNH (hPa)
SET:EGT:CAH:1500     # EGT caution high (raw ×10)
SET:CHT:CAH:1100     # CHT caution high (raw ×10)
```

Full parameter reference: `docs/can-param-mnemonics.md`.

## SD card logging

Log files are stored under a folder named after the device ID (4-digit uppercase hex):

```
/<DevID>/<DevID>_YYYYMMDD.csv   ← dated file (after TIME: received)
/<DevID>/<DevID>_NODATE.csv     ← pre-date file
/<DevID>/startup.log
```

CSV columns: `aircraft_id, timestamp, can_id, len, b0, b1, b2, b3, b4, b5, b6, b7`

- `timestamp` is `YYYY-MM-DD HH:MM:SS` when the RTC is set, or `T+<ms>` before that.
- Each unique CAN ID is written at most once per 500 ms.

## EEPROM layout

Base address: `0x0100` (256).

| Offset | Type | Default | Description |
|--------|------|---------|-------------|
| 0x00 | `uint8_t` | `0xCB` | Magic byte — detects uninitialised EEPROM |
| 0x01 | `uint8_t` | `2` | CAN speed index: 0=125 / 1=250 / 2=500 kbps |
| 0x02 | `uint16_t` LE | `0x0001` | Device ID |
| 0x04 | `uint16_t` LE | `0x0000` | Aircraft ID |

## Message store

Up to 12 unique CAN IDs tracked in RAM simultaneously. New IDs beyond the limit are
silently dropped. Use `CLEAR` to reset.

## Build number

The firmware version is `v<MAJOR>.<MINOR>.<BUILD>`. `MAJOR` and `MINOR` are set in
`src/version.h`. `BUILD` auto-increments on every PlatformIO build via
`increment_build.py` and is stored in `src/build_number.h` (git-ignored; resets to 0
on a fresh clone).

The version is shown on LCD line 2 during boot and printed on serial after
`CANBUS_MONITOR_STARTED`. Use it to confirm a successful flash.

## Memory usage (build 18, Arduino Uno ATmega328P)

| Resource | Used | Total | Percentage |
|---|---|---|---|
| Flash | 29 414 bytes | 32 256 bytes | 91.2% |
| RAM (static) | 1 631 bytes | 2 048 bytes | 79.6% |

Avoid adding string literals without the `F()` macro. RAM headroom is approximately
417 bytes; the stack and heap share this space, so deep call chains or large local buffers
will cause silent corruption. Any significant feature addition should be weighed against a
RAM audit.

### Primary RAM consumers

| Item | Approximate size |
|---|---|
| `CanMessageStore` (12 entries × ~12 bytes) | ~144 bytes |
| LCD buffer + I2C library state | ~64 bytes |
| SD library buffers | ~512 bytes |
| `SdLogger` struct | ~32 bytes |
| Serial receive buffer | 64 bytes |
| Stack (estimated, varies) | ~200–300 bytes |
