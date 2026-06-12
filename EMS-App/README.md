# EMS-App

Cross-platform (.NET 10, Windows/Linux) host application for the CanbusMonitor. Connects
over USB serial, receives live CAN frame data, and manages device configuration.

## Projects

| Project | Role |
|---|---|
| `EmsApp.Core` | Shared types (`CanFrame`, `DeviceCommand`) — no I/O, no dependencies |
| `EmsApp.Serial` | `DeviceConnection` — wraps `SerialPort`, parses `CAN:` lines, sends commands |
| `EmsApp.Api` | ASP.NET Core minimal-API web service; exposes device management over HTTP |
| `EmsApp.Console` | CLI tool (`ems`); live monitor mode and one-shot command dispatch |

## Build

```bash
dotnet build EmsApp.sln
```

Requires .NET 10 SDK.

## Console tool

```bash
dotnet run --project src/EmsApp.Console -- <port> [command [args]]
```

| Example | Effect |
|---|---|
| `ems COM3` | Live monitor — print every received CAN frame |
| `ems COM3 time` | Set RTC to current UTC time |
| `ems COM3 devid 0042` | Set device ID to 0x0042 |
| `ems COM3 aircraft PH42` | Set aircraft ID to 0xPH42 — use hex |
| `ems COM3 speed 500` | Set CAN bus speed to 500 kbps |
| `ems COM3 clear` | Clear message store |
| `ems COM3 list` | Dump tracked IDs |
| `ems COM3 reset` | Software-reset the device |
| `ems COM3 download 20260610` | Download and print the log file for 2026-06-10 |

On Linux replace `COM3` with `/dev/ttyUSB0` or equivalent.

## Web service

```bash
dotnet run --project src/EmsApp.Api
```

Listens on `http://localhost:5100` by default. Configure port in `src/EmsApp.Api/appsettings.json`.
Swagger UI available at `/swagger`.

### Endpoints

| Method | Path | Effect |
|---|---|---|
| GET | `/status` | Connection status |
| POST | `/command/time` | Set RTC to server UTC time |
| POST | `/command/devid/{id}` | Set device ID (decimal) |
| POST | `/command/aircraft/{id}` | Set aircraft ID (decimal) |
| POST | `/command/speed/{kbps}` | Set CAN bus speed |
| POST | `/command/clear` | Clear message store |
| POST | `/command/reset` | Reset device |
| GET | `/command/list` | Trigger LIST dump (response arrives via serial) |
| GET | `/download/{yyyyMMdd}` | Trigger log download (data arrives via serial) |

## Serial protocol

The device speaks at 115200 8N1. Every received CAN frame is emitted as:
```
CAN:0x<canId_hex>:<data_hex>
```

Management commands are plain text lines terminated with `\n` — see the CanbusMonitor
readme for the full command reference.
