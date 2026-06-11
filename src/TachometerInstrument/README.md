# TachometerInstrument

Standalone Arduino Nano tachometer for aircraft engine monitoring. Reads RPM from a magneto or ignition pickup, shows the value on a local SSD1306 OLED display (128×64), and broadcasts it over CAN bus.

Designed to work as an independent instrument — it functions without any other unit on the bus. Multiple tachos can share the same bus by assigning each a different CAN message ID (configured via EEPROM).

---

## Hardware

| Component | Detail |
|-----------|--------|
| MCU | Arduino Nano (ATmega328) |
| CAN controller | MCP2515 (SPI, CS on D10) |
| Display | SSD1306 OLED 128×64, I2C address 0x3C |
| RPM input | D2 (INT0 hardware interrupt) |

### Wiring

```
Magneto P-lead  ──[RC filter]──> D2 (INT0)   RPM pulse input
MCP2515 CS                    -> D10          CAN bus chip select
MCP2515 MOSI                  -> D11
MCP2515 MISO                  -> D12
MCP2515 SCK                   -> D13
OLED SDA                      -> A4
OLED SCL                      -> A5
```

**I2C address:** Most SSD1306 128×64 modules default to `0x3C`. If the display does not initialise, check the SA0 solder pad on the module — bridging it changes the address to `0x3D`. Update `OLED_I2C_ADDR` in `lib/systemConfig/PinsMap.h` accordingly.

**Input signal:** The RPM input expects an active-low (open-collector) pulse, typically the P-lead output of a magneto or the points signal of an ignition module. An RC low-pass filter (e.g. 1 kΩ + 100 nF) on the input is strongly recommended to suppress ignition noise.

If your signal is active-high, change `RPM_PULSE_TRIGGER` in `lib/systemConfig/PinsMap.h` from `FALLING` to `RISING`.

---

## EEPROM settings

All configuration is stored in EEPROM and survives power cycles. On first boot (blank chip), defaults are written automatically. Settings can be changed via CAN bus without reflashing — see [Remote configuration](#remote-configuration) below.

| Address | Name | Type | Default | Description |
|---------|------|------|---------|-------------|
| 0x0100 | Magic byte | uint8 | 0xAC | Set on first boot. Reset to 0xFF to re-apply defaults. |
| 0x0101 | Pulses per revolution | uint8 | 1 | Number of pulses the pickup produces per engine revolution. |
| 0x0102 | Max RPM | uint16 LE | 2800 | Over-speed threshold. Display warns and CAN flag is set when exceeded. |
| 0x0104 | CAN ID | uint16 LE | 0x0C0 | CAN message ID used when broadcasting RPM. |

### Pulses per revolution

This depends on how the tachometer signal is derived:

| Source | Typical value |
|--------|--------------|
| Single-fire magneto (one contact set) | 1 |
| Dual-fire magneto or distributor ignition | 2 |
| Hall-effect sensor on crankshaft (4-cylinder 4-stroke) | 2 (one pulse per firing) |

When in doubt, set to 1, hold the engine at a known RPM (e.g. 1000 RPM static), and verify the displayed value. If the display reads double, set to 2.

---

## CAN bus

### Message format

Each tachometer transmits a 4-byte CAN frame every second.

| Byte | Content |
|------|---------|
| 0–1 | RPM as uint16, little-endian (0–9999) |
| 2 | Flags: bit 0 = over-speed, bit 1 = sensor active |
| 3–7 | Reserved (0x00) |

The `TACH_FLAG_ACTIVE` bit (bit 1) is always set while the firmware is running, allowing receivers to detect a silent/dead unit.

### CAN speed

500 kbps — must match all other devices on the bus.

---

## Startup sequence

On every boot the firmware runs the following steps before entering the main loop:

1. **Loopback self-test.** The MCP2515 is initialised in `MODE_LOOPBACK`. A test frame (CAN ID `0x7FF`, data `A5 5A 42 01`) is sent and received back. ID and data are verified.

2. **Failure path.** If the test fails, `CAN bus FAILED` is printed to serial and `busError` is set. The tachometer continues reading RPM and updating the local display, but no CAN frames are sent.

3. **Switch to normal mode.** On success the controller switches to `MODE_NORMAL`.

4. **SYSTEM_INIT broadcast.** A single `SYSTEM_INIT` frame is transmitted:

   | Field | Value |
   |-------|-------|
   | CAN ID | `0x7F0` |
   | Byte 0 | `0x01` (TachometerInstrument) |
   | Byte 1–2 | Configured tachometer CAN ID, little-endian (e.g. `C0 00` for `0x0C0`) |

After this the tachometer enters the main loop and broadcasts RPM every second.

Serial output on a clean boot:
```
TACHOMETER_STARTED
Pulses/rev: 1
Max RPM:    2800
CAN ID:     0xC0
CAN bus OK
```

---

## Using two tachometers (one per magneto)

Each unit runs identical firmware. The only difference is the CAN ID stored in EEPROM.

1. Flash both boards with the same firmware.
2. Power on the first board. It boots with the default CAN ID `0x0C0` (magneto 1).
3. Power on the second board with only it connected to the bus. Send the following CAN message to assign it CAN ID `0x0C1` (magneto 2):

   ```
   CAN ID : 0x201  (CONFIGURE_SET_WORD)
   Data   : 04 01 C1 00    (address 0x0104, value 0x00C1)
   ```

4. The second board stores the new ID in EEPROM, confirms via a `0x211` response, and uses `0x0C1` from then on.

Both boards can now run on the same bus simultaneously.

---

## Remote configuration

EEPROM values can be read and written via CAN bus using the shared configuration protocol. This is the same protocol used by EngineMonitor, so any existing host tool works with both.

### Write a byte

```
CAN ID : 0x200
Data   : [addr_lo] [addr_hi] [value]
```

### Write a word (2 bytes, little-endian)

```
CAN ID : 0x201
Data   : [addr_lo] [addr_hi] [val_lo] [val_hi]
```

### Read a byte

```
CAN ID : 0x202
Data   : [addr_lo] [addr_hi]
Response (CAN ID 0x210): [addr_lo] [addr_hi] [value]
```

### Read a word

```
CAN ID : 0x203
Data   : [addr_lo] [addr_hi]
Response (CAN ID 0x211): [addr_lo] [addr_hi] [val_lo] [val_hi]
```

### Common configuration examples

Set pulses per revolution to 2:
```
CAN 0x200 : 01 01 02
```

Set max RPM to 3200 (0x0C80):
```
CAN 0x201 : 02 01 80 0C
```

Set CAN ID to 0x0C1 (magneto 2):
```
CAN 0x201 : 04 01 C1 00
```

Reset to defaults (clears magic byte, defaults rewritten on next boot):
```
CAN 0x200 : 00 01 FF
```

---

## Build and upload

```bash
# Build
C:\Users\gjkaa\.platformio\penv\Scripts\pio run

# Upload
C:\Users\gjkaa\.platformio\penv\Scripts\pio run --target upload

# Serial monitor (9600 baud)
C:\Users\gjkaa\.platformio\penv\Scripts\pio device monitor
```

Run these commands from the `src/TachometerInstrument/` directory.

On startup, the serial monitor prints the active configuration:

```
TACHOMETER_STARTED
Pulses/rev: 1
Max RPM:    2800
CAN ID:     0xC0
CAN bus OK
```
