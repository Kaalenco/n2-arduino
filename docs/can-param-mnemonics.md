# CAN Configuration Parameter Mnemonics

All `SET` serial commands and CAN CONFIG messages use 3-character parameter mnemonics.
Full protocol specification: [ADR 00008](adr/00008-bus-command-and-configuration-protocol.md).

## Serial command format

```
SET:<TYPE>:<PARAM>:<VALUE>
```

Examples:

```
SET:RPM:GRL:700
SET:RPM:GRH:2500
SET:RPM:RED:2800
SET:ALT:QNH:1013
SET:EGT:CAH:1500
SET:CHT:CAH:1100
```

## Mnemonic table

| Type  | Mnemonic | Full name        | Unit       | Notes                              |
|-------|----------|------------------|------------|------------------------------------|
| `RPM` | `GRL`    | Green arc Low    | RPM        | Lower bound of normal operating range |
| `RPM` | `GRH`    | Green arc High   | RPM        | Upper bound of normal operating range |
| `RPM` | `RED`    | Red line         | RPM        | Overspeed / do-not-exceed limit    |
| `ALT` | `QNH`    | QNH              | hPa        | Sea-level pressure, altimeter set  |
| `EGT` | `CAL`    | Caution Low      | °C × 10   | Lower caution threshold (0.1 °C)   |
| `EGT` | `CAH`    | Caution High     | °C × 10   | Upper caution threshold (0.1 °C)   |
| `CHT` | `CAL`    | Caution Low      | °C × 10   | Lower caution threshold (0.1 °C)   |
| `CHT` | `CAH`    | Caution High     | °C × 10   | Upper caution threshold (0.1 °C)   |

## Adding a new parameter

1. Choose a mnemonic following the convention above. Check this table for conflicts.
2. Add the entry to this table.
3. Add the instrument target-type code to the registry in ADR 00008 if it is a new type.
4. Add the case to `lookupConfig()` in `src/CanbusMonitor/src/main.cpp`.
5. Add a typed helper in `EMS-App/src/EmsApp.Core/DeviceCommand.cs`.
