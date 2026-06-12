# 00008. Bus command and configuration protocol

2026-06-12

## Status

Accepted

## Context

As more instruments join the CanFIX bus, the ground station (CanbusMonitor) needs a way to:

1. Broadcast a bus-wide reset so all nodes return to a known state after a firmware flash or
   fault condition.
2. Push display-range configuration to specific instrument types — for example, green-arc limits
   for an RPM display, or the current QNH setting for a barometric altimeter.

Configuration data originates in the EMS-App (a .NET 10 PC application), is transmitted over
USB serial to the CanbusMonitor, and is forwarded onto the CAN bus as a CONFIG message addressed
to the relevant instrument type.

CAN IDs `0x7E0–0x7EF` are reserved for this command layer.  Data IDs for instrument measurements
(`0x0C0`, `0x0D0`, etc.) are defined separately.

## Decision

### CAN ID assignments

| CAN ID  | Name     | Direction            | Payload |
|---------|----------|----------------------|---------|
| `0x7E0` | CONFIG   | Monitor → Instruments | 8 bytes — see frame layout below |
| `0x7EF` | SYSRESET | Monitor → All nodes  | 0 bytes |

All other IDs in `0x7E1–0x7EE` are reserved for future command types.

### CONFIG frame layout

```
Byte 0   target_type   Instrument category; 0x00 = broadcast to all
Byte 1   param_id      Parameter within that type's namespace
Byte 2   value_lo      uint16 little-endian LSB
Byte 3   value_hi      uint16 little-endian MSB
Byte 4–7 reserved      Must be 0x00
```

### Target type registry

| Code   | Name       | Description                              |
|--------|------------|------------------------------------------|
| `0x00` | BROADCAST  | All instrument nodes                     |
| `0x01` | RPM        | Tachometer / RPM display                 |
| `0x02` | ALTIMETER  | Barometric altimeter                     |
| `0x03` | EGT        | Exhaust gas temperature display          |
| `0x04` | CHT        | Cylinder head temperature display        |
| `0x10` | CANMOCK    | Development mock node (not production)   |

### Parameter namespaces

Parameter names are 3-character mnemonics. Full list with descriptions: `docs/can-param-mnemonics.md`.

**RPM (0x01)** — values in RPM

| param_id | Mnemonic | Description                |
|----------|----------|----------------------------|
| `0x01`   | `GRL`    | Green arc low              |
| `0x02`   | `GRH`    | Green arc high             |
| `0x03`   | `RED`    | Redline / overspeed limit  |

**Altimeter (0x02)** — value in hPa

| param_id | Mnemonic | Description                        |
|----------|----------|------------------------------------|
| `0x01`   | `QNH`    | Sea-level pressure (altimeter set) |

**EGT (0x03)** — values in °C × 10 (0.1 °C resolution)

| param_id | Mnemonic | Description        |
|----------|----------|--------------------|
| `0x01`   | `CAL`    | Caution low limit  |
| `0x02`   | `CAH`    | Caution high limit |

**CHT (0x04)** — values in °C × 10 (0.1 °C resolution)

| param_id | Mnemonic | Description        |
|----------|----------|--------------------|
| `0x01`   | `CAL`    | Caution low limit  |
| `0x02`   | `CAH`    | Caution high limit |

### Serial interface extension (EMS-App → CanbusMonitor)

```
SYSRESET                         → transmit CAN 0x7EF (no payload)
SET:<TYPE>:<PARAM>:<VALUE>       → transmit CAN 0x7E0 with resolved type/param codes
```

Examples:

```
SYSRESET
SET:RPM:GRL:700
SET:RPM:GRH:2500
SET:RPM:RED:2800
SET:ALT:QNH:1013
SET:EGT:CAH:1500
SET:CHT:CAH:1100
```

### Instrument contract

Each instrument node must:
- Define its `INSTRUMENT_TYPE` constant matching the registry above.
- In the main loop, poll the CAN bus for incoming messages.
- On receipt of `0x7EF`: perform a software reset.
- On receipt of `0x7E0` where `data[0] == INSTRUMENT_TYPE || data[0] == 0x00`:
  - Apply the parameter to runtime state.
  - Persist to EEPROM so the value survives power cycles.
  - Acknowledge receipt via Serial (debug only; no CAN ACK required).

The CanMock node (`0x10`) handles SYSRESET only.  It logs CONFIG messages but does not
apply or persist them because its sole purpose is generating test traffic, not displaying
instrument data.

## Consequences

- CanbusMonitor firmware gains two new CAN TX functions (`sendBusReset`, `sendConfig`) and
  two new serial commands (`SYSRESET`, `SET:<TYPE>:<PARAM>:<VALUE>`).
- Instrument firmware (to be written) must implement the CONFIG receive loop and EEPROM
  persistence for each supported parameter.
- The EMS-App gains typed `DeviceCommand` helpers and matching API / CLI endpoints.
- Adding a new instrument type requires only: a new type code in the registry, a new
  parameter namespace, and the corresponding firmware receive logic.
- The protocol is intentionally simple (no CAN ACK, no sequence numbers) — configuration
  is idempotent and can be re-sent without side effects.
