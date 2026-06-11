# EMS Project - Engine Monitor System

## Project Identification
- **Name**: EMS (Engine Monitor System)
- **Documentation**: `C:\git\n2-arduino\docs\Engine-Monitor-System.md`
- **Code Location**: `C:\git\n2-arduino\EngineMonitor`
- **Platform**: PlatformIO with Arduino framework
- **Target Board**: Arduino Nano ATmega328 (32KB Flash, 2KB RAM)

## Purpose
Safety-critical subsystem for real-time monitoring of aircraft engine parameters. Reads temperature, pressure, and RPM data at high frequency (10Hz) and provides immediate fault detection and alerting capabilities.

## Hardware Components

### Temperature Sensors (9× MAX6675 K-type thermocouple interfaces)
- **4× EGT** (Exhaust Gas Temperature): 0-900°C range
  - CS Pins: D10, D11, D12, D13
- **4× CHT** (Cylinder Head Temperature): 0-300°C range
  - CS Pins: A0, A1, A2, A3 (used as digital)
- **1× Oil Temperature**: 0-150°C range
  - CS Pin: D9

### Pressure Sensors (Analog)
- **Oil Pressure**: 0-100 PSI on A4
- **Manifold Pressure**: 10-30 inHg on A5 (for turbo monitoring)

### Other Sensors/Components
- **Tachometer**: Frequency counter on D2 (INT0), 0-6000 RPM
- **RTC**: DS3231 (I2C) for engine hours tracking
- **CAN Controller**: MCP2515 (SPI) with TJA1050 transceiver
- **Status LED**: Fault indication

## CAN Bus Protocol
- **CAN ID Range**: 0x100-0x1FF
- **Transmitted Messages**:
  - 0x100: EGT Temperatures (10Hz)
  - 0x101: CHT Temperatures (10Hz)
  - 0x102: Oil & Manifold (10Hz)
  - 0x103: Tach & Hours (1Hz)
  - 0x104: Engine Status (1Hz)
  - 0x10F: Heartbeat (1Hz)
- **Received Messages**:
  - 0x400: System Config
  - 0x401: Time Sync

## Key Features
1. **High-Frequency Sensor Reading**: 10Hz (100ms) update rate
2. **Engine Hours Tracking**: Accumulates when RPM > 500, persists to EEPROM every 0.1 hour
3. **Limit Checking and Alerts**:
   - EGT: Warn >750°C, Critical >800°C
   - CHT: Warn >200°C, Critical >230°C
   - Oil Temp: Warn >120°C, Critical >130°C
   - Oil Pressure: Warn <30 PSI, Critical <20 PSI
   - RPM: Warn >2700, Critical >2900
4. **CAN Bus Communication**: Transmits all data at required intervals
5. **Watchdog Timer**: Hardware watchdog for safety

## PlatformIO Configuration
```ini
[env:nanoatmega328]
platform = atmelavr
board = nanoatmega328
framework = arduino
lib_deps = 
	coryjfowler/mcp_can@^1.5.1
	adafruit/MAX6675 library@^1.1.2
	adafruit/RTClib@^2.1.4
```

## Current Status
- Project skeleton created with basic serial initialization
- Main source file: `EngineMonitor/src/main.cpp`
- Dependencies configured in platformio.ini
- Needs full implementation of sensor reading, CAN communication, and alert logic

## Reusable Code from Main System
The main n2-arduino project at `C:\git\n2-arduino` contains reusable patterns:
- Event-driven architecture (EventManager library)
- MAX6675 sensor reading patterns (extend from 4 to 9 sensors)
- Memory management patterns (F() macro, sprintf to buffers)
- Clock/RTC handling (DS3231)

## Memory Budget
- **Flash**: ~18KB of 32KB used (56%), 14KB available
- **RAM**: ~1,248 bytes of 2,048 used (61%), 800 bytes available
- **EEPROM**: 68 bytes of 1,024 used
  - 0x00-0x03: Engine hours
  - 0x04-0x23: Alert limit configuration
  - 0x24-0x43: Calibration data

## Power Budget
- **Active**: 76mA @ 5V = 380mW
- **Idle**: 27mA @ 5V = 135mW
- **Peak**: 126mA @ 5V = 630mW

## Integration Points
- **To Data Logger**: All engine data and alerts
- **To Pilot Information**: Real-time display and warnings
- **From Pilot Information**: Configuration and time sync
