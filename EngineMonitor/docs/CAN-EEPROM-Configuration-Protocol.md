# CAN Bus EEPROM Configuration Protocol

This document describes the CAN bus protocol for remotely reading and writing EEPROM configuration values on the Engine Monitor System.

## Overview

The EEPROM Configuration Protocol enables remote systems to:
- Read calibration values and alarm thresholds
- Write new configuration values
- Adjust sensor offsets without physical access to the device

All communication uses standard CAN 2.0A frames with 11-bit identifiers.

## CAN Bus Settings

| Parameter | Value |
|-----------|-------|
| Bus Speed | 500 kbps |
| Clock | 8 MHz (MCP2515) |
| Frame Format | Standard (11-bit ID) |
| Data Length | 2-4 bytes depending on message type |

## Message Types

### Request Messages (Remote → Engine Monitor)

| Message | ID | Description |
|---------|-----|-------------|
| `CONFIGURE_SET_BYTE` | 0x200 | Write a single byte to EEPROM |
| `CONFIGURE_SET_WORD` | 0x201 | Write a 2-byte word to EEPROM |
| `CONFIGURE_GET_BYTE` | 0x202 | Request a single byte from EEPROM |
| `CONFIGURE_GET_WORD` | 0x203 | Request a 2-byte word from EEPROM |

### Response Messages (Engine Monitor → Remote)

| Message | ID | Description |
|---------|-----|-------------|
| `CONFIGURE_BYTE_VALUE` | 0x210 | Response containing a single byte value |
| `CONFIGURE_WORD_VALUE` | 0x211 | Response containing a 2-byte word value |

## Data Formats

All multi-byte values use **little-endian** byte ordering (least significant byte first).

### CONFIGURE_SET_BYTE (0x200)

Write a single byte value to an EEPROM address.

| Byte | Field | Description |
|------|-------|-------------|
| 0 | addr_lo | Address low byte |
| 1 | addr_hi | Address high byte |
| 2 | value | Byte value to write |

**Response:** `CONFIGURE_BYTE_VALUE` with the written value as confirmation.

### CONFIGURE_SET_WORD (0x201)

Write a 2-byte word value to an EEPROM address.

| Byte | Field | Description |
|------|-------|-------------|
| 0 | addr_lo | Address low byte |
| 1 | addr_hi | Address high byte |
| 2 | val_lo | Value low byte |
| 3 | val_hi | Value high byte |

**Response:** `CONFIGURE_WORD_VALUE` with the written value as confirmation.

### CONFIGURE_GET_BYTE (0x202)

Request a single byte value from an EEPROM address.

| Byte | Field | Description |
|------|-------|-------------|
| 0 | addr_lo | Address low byte |
| 1 | addr_hi | Address high byte |

**Response:** `CONFIGURE_BYTE_VALUE` with the current value.

### CONFIGURE_GET_WORD (0x203)

Request a 2-byte word value from an EEPROM address.

| Byte | Field | Description |
|------|-------|-------------|
| 0 | addr_lo | Address low byte |
| 1 | addr_hi | Address high byte |

**Response:** `CONFIGURE_WORD_VALUE` with the current value.

### CONFIGURE_BYTE_VALUE (0x210)

Response containing a single byte value.

| Byte | Field | Description |
|------|-------|-------------|
| 0 | addr_lo | Address low byte |
| 1 | addr_hi | Address high byte |
| 2 | value | Byte value |

### CONFIGURE_WORD_VALUE (0x211)

Response containing a 2-byte word value.

| Byte | Field | Description |
|------|-------|-------------|
| 0 | addr_lo | Address low byte |
| 1 | addr_hi | Address high byte |
| 2 | val_lo | Value low byte |
| 3 | val_hi | Value high byte |

## EEPROM Memory Map

### Temperature Sensor Calibration Offsets

Stored as `int16_t` in degrees Celsius × 10 (e.g., -20.5°C = -205).

| Address | Description | Type |
|---------|-------------|------|
| 0x0151 | Ambient temperature offset | int16_t |
| 0x0153 | EGT offset | int16_t |
| 0x0155 | CHT1 offset | int16_t |
| 0x0157 | CHT2 offset | int16_t |
| 0x0159 | CHT3 offset | int16_t |
| 0x015B | CHT4 offset | int16_t |
| 0x015D | Oil temperature offset | int16_t |

### Temperature Alarm Thresholds

Stored as `int16_t` in degrees Celsius.

| Address | Description | Type |
|---------|-------------|------|
| 0x010A | Ambient temp warning high | int16_t |
| 0x010C | Ambient temp warning low | int16_t |
| 0x010E | EGT caution | int16_t |
| 0x0110 | EGT warning | int16_t |
| 0x0112 | CHT caution | int16_t |
| 0x0114 | CHT warning | int16_t |
| 0x0116 | Oil temp caution high | int16_t |
| 0x0118 | Oil temp warning high | int16_t |
| 0x011A | Oil temp warning low | int16_t |

### Pressure Alarm Thresholds

Oil pressure stored as `int16_t` in PSI × 10 (e.g., 25.5 PSI = 255).

| Address | Description | Type |
|---------|-------------|------|
| 0x011C | Oil pressure caution low | int16_t |
| 0x011E | Oil pressure warning low | int16_t |
| 0x0120 | Oil pressure warning high | int16_t |

### Fuel System Thresholds

Stored as percentage × 10 (e.g., 25.5% = 255).

| Address | Description | Type |
|---------|-------------|------|
| 0x0122 | Fuel warning low | int16_t |
| 0x0124 | Fuel caution low | int16_t |

### Electrical System Thresholds

Battery voltage stored as `int16_t` in volts × 100 (e.g., 12.5V = 1250).

| Address | Description | Type |
|---------|-------------|------|
| 0x0126 | Battery voltage warning low | int16_t |
| 0x0128 | Battery voltage warning high | int16_t |
| 0x012A | Battery current warning high | int16_t |

### Barometer/Altitude Settings

| Address | Description | Type |
|---------|-------------|------|
| 0x0104 | QNH setting (hPa × 10) | int16_t |
| 0x0106 | Altitude warning high (ft) | int16_t |
| 0x0108 | Altitude warning low (ft) | int16_t |

### System Configuration

| Address | Description | Type |
|---------|-------------|------|
| 0x0180 | Config flags 1 (alarm enables) | uint8_t |
| 0x0181 | Config flags 2 (units) | uint8_t |
| 0x0182 | LCD brightness | uint8_t |
| 0x0183 | LCD contrast | uint8_t |
| 0x0184 | Fast update interval (ms) | uint16_t |
| 0x0186 | Slow update interval (ms) | uint16_t |

#### Config Flags 1 (0x0180)

| Bit | Description |
|-----|-------------|
| 0 | Enable EGT alarms |
| 1 | Enable CHT alarms |
| 2 | Enable oil temp alarms |
| 3 | Enable oil pressure alarms |
| 4 | Enable fuel alarms |
| 5 | Enable battery voltage alarms |
| 6 | Enable altitude alarms |
| 7 | Audio alarm enable |

#### Config Flags 2 (0x0181)

| Bit | Description |
|-----|-------------|
| 0 | Temperature: 0=Celsius, 1=Fahrenheit |
| 1 | Altitude: 0=meters, 1=feet |
| 2 | Volume: 0=liters, 1=gallons |
| 3-7 | Reserved |

## Usage Examples

### Example 1: Read EGT Calibration Offset

Read the current EGT temperature offset from address 0x0153.

**Request:**
```
ID: 0x203 (CONFIGURE_GET_WORD)
Data: [0x53, 0x01]
       ^^^^  ^^^^
       addr_lo  addr_hi
```

**Response:**
```
ID: 0x211 (CONFIGURE_WORD_VALUE)
Data: [0x53, 0x01, 0x38, 0xFF]
       ^^^^  ^^^^  ^^^^  ^^^^
       addr_lo addr_hi val_lo val_hi

Value: 0xFF38 = -200 (signed) = -20.0°C offset
```

### Example 2: Set CHT Warning Threshold

Set CHT warning threshold to 230°C at address 0x0114.

**Request:**
```
ID: 0x201 (CONFIGURE_SET_WORD)
Data: [0x14, 0x01, 0xE6, 0x00]
       ^^^^  ^^^^  ^^^^  ^^^^
       addr_lo addr_hi val_lo val_hi

Value: 0x00E6 = 230°C
```

**Response (confirmation):**
```
ID: 0x211 (CONFIGURE_WORD_VALUE)
Data: [0x14, 0x01, 0xE6, 0x00]
```

### Example 3: Set LCD Brightness

Set LCD brightness to 200 at address 0x0182.

**Request:**
```
ID: 0x200 (CONFIGURE_SET_BYTE)
Data: [0x82, 0x01, 0xC8]
       ^^^^  ^^^^  ^^^^
       addr_lo addr_hi value (200)
```

**Response (confirmation):**
```
ID: 0x210 (CONFIGURE_BYTE_VALUE)
Data: [0x82, 0x01, 0xC8]
```

### Example 4: Enable All Alarms

Set config flags 1 to 0xFF (all alarms enabled) at address 0x0180.

**Request:**
```
ID: 0x200 (CONFIGURE_SET_BYTE)
Data: [0x80, 0x01, 0xFF]
```

**Response:**
```
ID: 0x210 (CONFIGURE_BYTE_VALUE)
Data: [0x80, 0x01, 0xFF]
```

## Implementation Notes

### Timing

- The Engine Monitor processes incoming CAN messages at the beginning of each main loop cycle
- Response messages are sent immediately after processing
- The main loop runs approximately every 1 second (configurable)

### Error Handling

- Invalid message lengths are silently ignored
- The system uses `EEPROM.update()` to minimize write cycles (only writes if value changed)
- No explicit error responses are sent; absence of response indicates failure

### Memory Safety

- All EEPROM addresses in the EMS memory map (0x0100-0x01FF) can be accessed
- Writing to addresses outside configured ranges may cause undefined behavior
- Always verify addresses against the memory map before writing

### CAN Bus Arbitration

- Configuration messages (0x200-0x211) have lower priority than engine data (0x100-0x103)
- Multiple configuration requests should be spaced to allow for responses

## Integration Example

### Python Example (using python-can)

```python
import can
import struct

# Setup CAN bus
bus = can.interface.Bus(channel='can0', bustype='socketcan')

def read_word(address):
    """Read a 2-byte value from EEPROM"""
    msg = can.Message(
        arbitration_id=0x203,
        data=[address & 0xFF, (address >> 8) & 0xFF],
        is_extended_id=False
    )
    bus.send(msg)

    # Wait for response
    response = bus.recv(timeout=1.0)
    if response and response.arbitration_id == 0x211:
        addr = response.data[0] | (response.data[1] << 8)
        value = response.data[2] | (response.data[3] << 8)
        # Convert to signed if needed
        if value > 32767:
            value -= 65536
        return value
    return None

def write_word(address, value):
    """Write a 2-byte value to EEPROM"""
    # Convert signed to unsigned
    if value < 0:
        value += 65536

    msg = can.Message(
        arbitration_id=0x201,
        data=[
            address & 0xFF,
            (address >> 8) & 0xFF,
            value & 0xFF,
            (value >> 8) & 0xFF
        ],
        is_extended_id=False
    )
    bus.send(msg)

    # Wait for confirmation
    response = bus.recv(timeout=1.0)
    return response is not None

# Example: Read EGT offset
egt_offset = read_word(0x0153)
print(f"EGT offset: {egt_offset / 10.0}°C")

# Example: Set new EGT offset (-15.5°C)
write_word(0x0153, -155)
```

## Related Documentation

- [EMSMemoryMap.h](../lib/systemConfig/EMSMemoryMap.h) - Complete EEPROM address definitions
- [EepromConfigure.h](../lib/eepromConfigure/EepromConfigure.h) - Implementation source code
- [Engine-Monitor-System.md](./Engine-Monitor-System.md) - System overview

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2025-01-21 | Initial protocol specification |
