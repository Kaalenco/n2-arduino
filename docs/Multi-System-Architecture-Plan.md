# Multi-System Embedded Architecture Planning Document
## Aircraft Instrumentation System Evolution

**Document Version:** 1.0
**Date:** 2025-01-18
**Project:** N2-Arduino Aircraft Instrumentation
**Author:** Embedded Systems Architecture Team

---

## Executive Summary

This document outlines the evolution of the current single-board Arduino Nano aircraft instrumentation system into a distributed four-subsystem architecture interconnected via CAN bus. The current system demonstrates solid foundational architecture with event-driven design, efficient memory management, and a well-abstracted sensor interface layer. The migration strategy preserves these strengths while addressing scalability, real-time requirements, and safety-critical needs inherent in aircraft instrumentation.

**Current System:** Arduino Nano ATmega328 (32KB Flash, 2KB RAM) running monolithic sensor acquisition and display system.

**Target System:** Four specialized subsystems:
1. **Engine Monitor** (Arduino-based) - Real-time engine parameters
2. **Instrument System** (Arduino-based) - Flight instruments and sensors
3. **Data Logger** (Raspberry Pi Zero) - Persistent storage and retrieval
4. **Pilot Information** (Raspberry Pi 3/4) - Display interface and configuration

**Key Benefits:**
- Separation of concerns for safety-critical functions
- Parallel processing of engine and flight data
- Robust data logging with non-volatile storage
- Enhanced user interface capabilities
- Modular design for maintenance and upgrades
- Fault isolation and redundancy options

---

## Current System Analysis

### Architecture Review

#### Strengths to Preserve

**1. Event-Driven Architecture**
- Clean separation between event handling and event generation
- `handleEvents()` → `raiseEvents()` pattern prevents queue overflow
- EventManager library (8-event queues, 8-listener lists) is well-sized for current needs
- Zero polling in main loop - all interactions are event-driven

**2. Memory Management Patterns**
- Excellent use of PROGMEM with `F()` macro for string literals
- Virtual screen system uses char arrays (not String objects)
- Sensor data stored in fixed-size int array
- sprintf() used for formatted output to char buffers
- Documented awareness of String concatenation dangers

**3. Library Structure**
- Well-abstracted hardware controls (ButtonControl, RotaryEncoder)
- Sensor interfaces encapsulated (Barometer, Clock, MAX6675)
- Clear separation between control logic and hardware
- Each library maintains its own EventManager instance

**4. Virtual Screen System**
- Clever memory-efficient approach to multi-screen UI
- 5 screens × 2 rows × 17 bytes = 170 bytes total (minimal overhead)
- Instant screen switching without rendering delay
- Content persistence across screen changes

**5. Sensor Integration Patterns**
- Consistent sensor reading approach in `handleClockTimeEvent()`
- Centralized data storage in `SensorData[]` array
- Well-defined sensor constants in SensorTypes.h
- Active/inactive state checking before sensor reads

#### Resource Utilization (Estimated from Code)

**Flash Memory Usage:**
- Core application logic: ~8-10KB
- Libraries (EventManager, I2C, SPI, LCD): ~12-15KB
- String literals (with PROGMEM): ~2-3KB
- **Estimated Total: 22-28KB of 32KB (69-87% utilization)**
- **Available for expansion: ~4-10KB**

**RAM Usage:**
- Virtual screens: 170 bytes (5 screens × 2 rows × 17 bytes)
- SensorData array: 28 bytes (14 × 2 bytes)
- Event queues: ~192 bytes (3 managers × 8 events × 8 bytes)
- Listener lists: ~192 bytes (3 managers × 8 listeners × 8 bytes)
- Stack/global variables: ~400 bytes
- Library buffers: ~300 bytes
- **Estimated Total: 1,282 bytes of 2,048 bytes (63% utilization)**
- **Available for expansion: ~766 bytes**

**I/O Utilization:**
- Digital pins: 8 of 14 used (D2-D7, D10-D13)
- Analog pins: 3 of 8 used (A0-A2)
- I2C bus: 3 devices (LCD, BMP085, RTC)
- SPI bus: 4 devices (MAX6675 sensors)
- **Good headroom for additional sensors**

#### Identified Issues to Address

**1. Memory Constraints**
- Arduino Nano approaching limits for complex features
- Virtual screen system limited to 5 screens (could expand but RAM constrained)
- No room for data logging or historical data storage
- Limited buffer space for communication protocols

**2. Processing Bottlenecks**
- Single-threaded execution limits concurrent sensor reading
- Engine parameters mixed with display updates (different timing requirements)
- No prioritization between critical (engine) and non-critical (display) tasks

**3. Safety and Reliability**
- Single point of failure - one board controls everything
- No data redundancy or backup logging
- Display failure could mask sensor failures
- No watchdog or fault detection mentioned in code

**4. Scalability Issues**
- Adding sensors requires careful RAM/Flash management
- No clear expansion path for advanced features (GPS, AHRS, fuel flow)
- User interface limited by 16×2 LCD constraints
- Configuration storage limited to 512 bytes EEPROM

**5. Real-Time Requirements**
- Engine monitoring (EGT, CHT) requires fast response (<100ms)
- Display updates less critical (200-500ms acceptable)
- Current 200ms loop delay may be too slow for engine protection
- No explicit real-time guarantees or deadline tracking

---

## System Architecture Design

### Hardware Platform Selection

#### Subsystem 1: Engine Monitor System
**Platform:** Arduino Nano 33 IoT or Arduino Uno

**Justification:**
- **Real-time requirements:** Engine monitoring is safety-critical, requiring deterministic response times
- **Sensor count:** 7-9 temperature sensors (EGT, CHT1-4, oil temp, intake temp)
- **Update rate:** 10Hz minimum (100ms) for protective monitoring
- **Power consumption:** ~25mA active, can use sleep modes between readings
- **Cost:** $15-25, proven reliability
- **I/O needs:** 9 SPI chip selects, 2 analog inputs (oil pressure, manifold pressure)

**Key Components:**
- ATmega328P or SAMD21 microcontroller
- MCP2515 CAN controller (SPI interface)
- 9× MAX6675 thermocouple interfaces
- Real-time clock for engine hours tracking
- EEPROM for engine hours persistence

#### Subsystem 2: Instrument System
**Platform:** Arduino Mega 2560 or Teensy 4.0

**Justification:**
- **Processing:** Multiple sensors with different protocols (I2C, SPI, UART, analog)
- **Real-time:** Flight instruments need 5Hz update rate (200ms)
- **Sensor diversity:** BMP390 (pressure/altitude), BMI088 (6-axis IMU), BNO055 (9-DOF), GPS module
- **Power consumption:** ~50mA active, important but not critical
- **I/O needs:** 2× I2C buses, 1× SPI bus, 4× analog, 2× UART
- **Cost:** $40-50 for Mega, $25 for Teensy
- **Memory:** 8KB RAM allows sensor fusion algorithms

**Key Components:**
- ATmega2560 (Mega) or ARM Cortex-M7 (Teensy)
- MCP2515 CAN controller
- BMP390 barometric pressure sensor
- BMI088 or MPU6050 IMU (gyro + accelerometer)
- BNO055 9-DOF sensor (magnetometer for heading)
- GPS module (UART)
- Fuel level sensor (analog)
- Battery monitoring (analog voltage divider)

#### Subsystem 3: Data Logging System
**Platform:** Raspberry Pi Zero 2 W

**Justification:**
- **Storage:** MicroSD card for gigabytes of logged data
- **Processing:** Can handle data parsing, compression, file management
- **Power:** ~200mA, must be properly shut down to avoid corruption
- **Cost:** $15, includes WiFi for wireless data retrieval
- **Development:** Python/C++ for logging daemon
- **Not real-time critical:** Logging can tolerate 1-2 second delays

**Key Components:**
- Quad-core ARM Cortex-A53 (1GHz)
- MicroSD card (32-128GB)
- MCP2515 CAN controller (SPI)
- USB CAN adapter alternative
- Power management circuit (safe shutdown)

#### Subsystem 4: Pilot Information System
**Platform:** Raspberry Pi 4 (2GB RAM)

**Justification:**
- **Display:** 7" touchscreen (800×480) or HDMI monitor
- **Processing:** Qt/GTK GUI with real-time graphs and multi-screen layouts
- **User input:** Touchscreen or USB peripherals
- **Configuration:** Rich UI for QNH setting, date/time, units, calibration
- **Not real-time critical:** Display updates at 2-5Hz acceptable
- **Power:** ~600mA with display, needs regulated 5V supply
- **Development:** Python/Qt for rapid UI development

**Key Components:**
- Quad-core ARM Cortex-A72 (1.5GHz)
- 2GB RAM (sufficient for GUI)
- Official 7" touchscreen or HDMI output
- MCP2515 CAN controller or USB-CAN adapter
- USB ports for keyboard/mouse during development

### System Interconnection Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        CAN Bus Network                          │
│                    500 kbps, 120Ω termination                   │
└────┬──────────────────┬───────────────────┬──────────────┬──────┘
     │                  │                   │              │
┌────▼──────────┐  ┌────▼──────────┐  ┌────▼────────┐  ┌──▼─────────┐
│  Engine       │  │  Instrument    │  │  Data       │  │  Pilot     │
│  Monitor      │  │  System        │  │  Logger     │  │  Info      │
│               │  │                │  │             │  │  System    │
│  Arduino      │  │  Arduino Mega  │  │  RPi Zero   │  │  RPi 4     │
│  Nano/Uno     │  │  or Teensy 4   │  │  2W         │  │  2GB       │
│               │  │                │  │             │  │            │
│  - EGT ×4     │  │  - Altitude    │  │  - microSD  │  │  - 7" LCD  │
│  - CHT ×4     │  │  - IMU/AHRS    │  │  - CSV log  │  │  - Touch   │
│  - Oil Temp   │  │  - GPS         │  │  - WiFi     │  │  - HDMI    │
│  - Oil Press  │  │  - Fuel Level  │  │  - CAN Rx   │  │  - Config  │
│  - Tach       │  │  - Battery     │  │  - Safe PWR │  │  - Graphs  │
│  - Eng Hours  │  │  - Ambient T   │  │             │  │  - Alerts  │
└───────────────┘  └────────────────┘  └─────────────┘  └────────────┘
     CAN ID              CAN ID              CAN ID           CAN ID
   0x100-0x1FF        0x200-0x2FF         0x300-0x3FF      0x400-0x4FF
```

### CAN Bus Topology and Network Design

**Physical Layer:**
- CAN 2.0B standard (500 kbps bit rate)
- Linear bus topology with 120Ω termination resistors at each end
- MCP2515 CAN controllers with TJA1050 transceivers
- Twisted pair cable (Cat5e or dedicated CAN cable)
- Maximum bus length: 100m @ 500kbps (1m sufficient for aircraft panel)
- 4 nodes (well below 30-node limit)

**Power Distribution:**
- Separate 5V regulators for each subsystem (fault isolation)
- Common ground reference for CAN_L/CAN_H signals
- Power filtering on each node to reduce CAN bus noise
- Engine Monitor and Instruments on aircraft 12V bus (with regulation)
- RPi systems on dedicated 5V supply (higher current requirement)

**Termination:**
- 120Ω resistors at Engine Monitor (far end) and Pilot Info (near end)
- No termination on middle nodes (Instrument System, Data Logger)

### Power Architecture and Consumption Estimates

**Engine Monitor System:**
- Arduino Nano: 20mA @ 5V = 100mW
- MCP2515 CAN: 5mA @ 5V = 25mW
- MAX6675 ×9: 50mA @ 5V = 250mW (reading, 1.5mA standby each)
- RTC: 1mA @ 5V = 5mW
- **Total: 76mA @ 5V = 380mW active, 27mA idle = 135mW**

**Instrument System:**
- Arduino Mega: 50mA @ 5V = 250mW
- MCP2515 CAN: 5mA @ 5V = 25mW
- BMP390: 3mA @ 3.3V = 10mW
- IMU (BMI088): 5mA @ 3.3V = 17mW
- GPS: 40mA @ 3.3V = 132mW
- Sensors (analog): 10mA @ 5V = 50mW
- **Total: 113mA @ 5V = 484mW active**

**Data Logger:**
- RPi Zero 2W: 200mA @ 5V = 1000mW (idle)
- MicroSD write: +50mA = 250mW (bursts)
- MCP2515 CAN: 5mA @ 5V = 25mW
- **Total: 255mA @ 5V = 1275mW (1.5W peak with SD writes)**

**Pilot Information:**
- RPi 4 (2GB): 600mA @ 5V = 3000mW
- 7" Display: 400mA @ 5V = 2000mW
- MCP2515 CAN: 5mA @ 5V = 25mW
- **Total: 1005mA @ 5V = 5025mW (5W)**

**System Total:**
- **Active (all systems): 1449mA @ 5V = 7.2 watts**
- **From 12V aircraft bus: ~640mA @ 12V (with 95% efficient regulators)**
- **Acceptable for aircraft electrical system (typical alternator: 60A @ 14V)**

**Power Efficiency Strategies:**
- Sleep modes on Arduino between sensor reads (save 60% on idle)
- RPi Zero uses cpufreq governor to scale CPU frequency
- Display brightness control (save 30-50% display power)
- SD card write caching (reduce write frequency)
- GPS power management (can reduce to 7mA in low-power mode)

### Fault Tolerance and Redundancy Strategy

**1. Independent Operation**
- Each subsystem can operate independently if CAN bus fails
- Engine Monitor continues protective monitoring with local display (future)
- Instrument System logs locally to EEPROM if logger unavailable
- Pilot Info shows "last known good" data if sensors offline

**2. Watchdog Timers**
- All Arduino systems: Hardware watchdog (15ms - 8s timeout)
- RPi systems: Software watchdog daemon monitoring critical processes
- CAN bus: Heartbeat messages every 1 second from each node
- Node failure detection within 3 seconds (3 missed heartbeats)

**3. Data Validation**
- Range checking on all sensor values before CAN transmission
- CRC checking on CAN messages (built into CAN protocol)
- Timeout detection: Display shows "---" if data >2 seconds old
- Sensor failure flags transmitted via CAN

**4. Fault Logging**
- Data Logger records all fault events with timestamps
- Persistent fault codes in EEPROM (survives power loss)
- Fault LED on each subsystem for quick visual diagnosis
- Detailed fault log retrievable via USB/WiFi

**5. Redundancy Options (Phase 2)**
- Dual temperature sensors on critical points (EGT, CHT)
- Backup altitude source (GPS altitude if barometer fails)
- Dual CAN buses for safety-critical messages
- Battery-backed RTC for time/date persistence

**6. Safe Failure Modes**
- Engine Monitor: Conservative limits if sensor suspect
- Instrument System: GPS altitude if barometer fails
- Data Logger: Queue in RAM if SD card fails, discard oldest
- Pilot Info: Show warnings, continue operation with reduced functionality

---

## Detailed Subsystem Specifications

### Subsystem 1: Engine Monitor System

**Hardware Platform:** Arduino Nano 33 IoT or Arduino Uno + MCP2515

**Key Components:**
- ATmega328P microcontroller (32KB Flash, 2KB RAM)
- MCP2515 CAN controller (SPI interface)
- TJA1050 CAN transceiver
- 9× MAX6675 K-type thermocouple interfaces (SPI, individual CS lines)
- DS3231 RTC (I2C) for engine hours tracking
- 2× Analog inputs (oil pressure sensor, manifold pressure sensor)
- 1× Frequency input (tachometer via interrupt pin)
- Status LED (engine fault indication)

**Sensor Interfaces:**
- **EGT Sensors (4×):** MAX6675, SPI, CS pins D10-D13, 0-900°C range
- **CHT Sensors (4×):** MAX6675, SPI, CS pins A0-A3, 0-300°C range
- **Oil Temperature:** MAX6675, SPI, CS pin D9, 0-150°C range
- **Oil Pressure:** Analog sensor, A4, 0-100 PSI (voltage divider)
- **Manifold Pressure:** Analog sensor, A5, 10-30 inHg (for turbo monitoring)
- **Tachometer:** Frequency input, D2 (INT0), 0-6000 RPM
- **Engine Hours:** DS3231 RTC, I2C, tracks cumulative running time

**Processing Responsibilities:**
- Read all 9 temperature sensors every 100ms (10Hz update rate)
- Calculate RPM from tachometer frequency input
- Track engine hours (increment when RPM > 500)
- Perform limit checking and generate alerts:
  - EGT: Warn >750°C, Critical >800°C
  - CHT: Warn >200°C, Critical >230°C
  - Oil Temp: Warn >120°C, Critical >130°C
  - Oil Pressure: Warn <20 PSI @ cruise RPM
- Transmit engine data on CAN bus every 100ms
- Flash fault LED if critical limit exceeded
- Store engine hours to EEPROM every 0.1 hour

**CAN Bus Messages (Transmitted):**

| Message ID | Name | Frequency | Data Layout |
|------------|------|-----------|-------------|
| 0x100 | EGT Temperatures | 10Hz | Byte 0-1: EGT1 (°C), 2-3: EGT2, 4-5: EGT3, 6-7: EGT4 |
| 0x101 | CHT Temperatures | 10Hz | Byte 0-1: CHT1 (°C), 2-3: CHT2, 4-5: CHT3, 6-7: CHT4 |
| 0x102 | Oil & Manifold | 10Hz | Byte 0-1: Oil Temp (°C), 2-3: Oil Pressure (PSI×10), 4-5: MAP (inHg×10) |
| 0x103 | Tach & Hours | 1Hz | Byte 0-1: RPM, 2-5: Engine Hours (seconds) |
| 0x104 | Engine Status | 1Hz | Byte 0: Status flags, 1: Max EGT, 2: Max CHT, 3-7: Reserved |
| 0x10F | Heartbeat | 1Hz | Byte 0: Node ID (0x01), 1: Uptime (minutes), 2-7: Reserved |

**CAN Bus Messages (Received):**

| Message ID | Name | Purpose |
|------------|------|---------|
| 0x400 | System Config | Receive alert limits, units configuration |
| 0x401 | Time Sync | Synchronize RTC with Pilot Info system |

**Power Budget:**
- Active: 76mA @ 5V (380mW)
- Idle: 27mA @ 5V (135mW)
- Peak: 126mA @ 5V (during all sensor reads simultaneously)

**Software Architecture:**
- **Reuse from current system:**
  - Event-driven architecture (EventManager library)
  - MAX6675 sensor reading patterns
  - EEPROM storage approach
  - sprintf() formatting for data preparation

- **New development:**
  - CAN bus driver integration (mcp_can library)
  - Tachometer frequency measurement (interrupt-based)
  - Engine hours tracking logic
  - Limit checking and alert generation
  - Watchdog timer implementation

**Memory Budget:**
- Flash: ~18KB (CAN library 6KB, existing patterns 12KB)
- RAM: 1.2KB (CAN buffers 256B, sensor data 128B, stack 800B)
- EEPROM: 64 bytes (engine hours, calibration data)

---

### Subsystem 2: Instrument System

**Hardware Platform:** Arduino Mega 2560 or Teensy 4.0

**Key Components:**
- ATmega2560 (16MHz, 256KB Flash, 8KB RAM) or Teensy (600MHz ARM Cortex-M7)
- MCP2515 CAN controller + TJA1050 transceiver
- BMP390 barometric pressure sensor (I2C, 0x77)
- BMI088 6-axis IMU (I2C, 0x18/0x19, accel + gyro)
- BNO055 9-DOF sensor (I2C, 0x28, magnetometer for heading)
- NEO-6M GPS module (UART, 9600 baud)
- Fuel level sensor (analog, 0-100% float sensor)
- Battery voltage sensor (analog, voltage divider 0-20V)
- Ambient temperature sensor (I2C or 1-Wire DS18B20)
- Pitot tube (analog pressure sensor for airspeed) - future

**Sensor Interfaces:**
- **Altitude/Pressure:** BMP390, I2C, ±1m resolution, 300-1100 hPa range
- **Attitude (pitch/roll):** BMI088, I2C, ±16g accel, ±2000°/s gyro
- **Heading (magnetic):** BNO055, I2C, 360° with 1° resolution
- **GPS Position:** NEO-6M, UART, lat/lon/altitude/speed/time
- **Fuel Level:** Resistive float sensor, A0, 0-100% (0-5V range)
- **Battery Voltage:** Voltage divider, A1, 0-20V range (scaled to 0-5V)
- **Ambient Temperature:** DS18B20, 1-Wire on D22, -55 to +125°C
- **Internal Temperature:** BMP390 built-in sensor

**Processing Responsibilities:**
- Altitude calculation from pressure (standard atmosphere model)
- QNH calibration support (set sea-level pressure)
- GPS NMEA sentence parsing (RMC, GGA messages)
- Sensor fusion: Combine IMU + magnetometer for stable heading
- Fuel flow calculation (rate of change of fuel level)
- Battery current estimation (from voltage change rate)
- Detect and flag sensor failures
- Transmit all instrument data on CAN bus
- Log critical events to internal EEPROM

**CAN Bus Messages (Transmitted):**

| Message ID | Name | Frequency | Data Layout |
|------------|------|-----------|-------------|
| 0x200 | Altitude & Pressure | 5Hz | Byte 0-1: Altitude (m), 2-3: Pressure (hPa×10), 4-5: QNH (hPa×10) |
| 0x201 | Attitude | 5Hz | Byte 0-1: Pitch (°×10), 2-3: Roll (°×10), 4-5: Heading (°) |
| 0x202 | GPS Position | 1Hz | Byte 0-3: Latitude (°×1e6), 4-7: Longitude (°×1e6) |
| 0x203 | GPS Velocity | 1Hz | Byte 0-1: Ground speed (km/h×10), 2-3: Track (°), 4: Satellites, 5: Fix quality |
| 0x204 | Fuel & Battery | 2Hz | Byte 0: Fuel % (0-100), 1-2: Battery V (×100), 3-4: Battery A (×100) |
| 0x205 | Temperature | 2Hz | Byte 0-1: Ambient (°C×10), 2-3: Internal (°C×10) |
| 0x206 | Sensor Status | 1Hz | Byte 0: Status flags (GPS fix, IMU cal, etc.), 1-7: Reserved |
| 0x20F | Heartbeat | 1Hz | Byte 0: Node ID (0x02), 1: Uptime (minutes), 2-7: Reserved |

**CAN Bus Messages (Received):**

| Message ID | Name | Purpose |
|------------|------|---------|
| 0x400 | System Config | Receive QNH setting, units, calibration values |
| 0x401 | Time Sync | Synchronize internal clock (GPS provides time but sync needed) |

**Power Budget:**
- Active: 113mA @ 5V (565mW)
- GPS acquisition: +40mA (initial fix, drops to 25mA tracking)
- Peak: 153mA @ 5V during GPS acquisition

**Software Architecture:**
- **Reuse from current system:**
  - Event-driven architecture
  - Barometer reading patterns (upgrade BMP085 → BMP390)
  - Clock/RTC time handling
  - sprintf() formatting
  - Sensor data array pattern

- **New development:**
  - CAN bus integration
  - GPS NMEA parser
  - IMU sensor fusion (Madgwick or complementary filter)
  - Magnetometer calibration (hard/soft iron)
  - Fuel flow rate calculation
  - Multi-sensor coordination logic

**Memory Budget:**
- Flash: ~60KB (CAN 6KB, GPS 8KB, IMU fusion 12KB, existing 34KB)
- RAM: 3.2KB (CAN 256B, GPS 512B, IMU 1KB, sensor data 512B, stack 1KB)
- EEPROM: 128 bytes (QNH, calibration, persistent config)

---

### Subsystem 3: Data Logging System

**Hardware Platform:** Raspberry Pi Zero 2 W

**Key Components:**
- BCM2837 SoC (4× ARM Cortex-A53 @ 1GHz)
- 512MB RAM
- MicroSD card (32GB minimum, 128GB recommended)
- MCP2515 CAN controller (SPI on GPIO pins)
- TJA1050 CAN transceiver
- Safe shutdown button (GPIO input with hardware debounce)
- Status LEDs (logging active, SD write, fault)
- WiFi 802.11n (on-chip)
- Real-time clock battery backup (DS3231 on I2C)

**Storage Strategy:**
- **Primary:** CSV files on microSD, one file per flight session
- **Format:** Timestamp, Message ID, Data (human-readable)
- **Rotation:** New file on engine start or every 2 hours
- **Retention:** Keep all files, user purges via WiFi interface
- **File naming:** `flight_YYYYMMDD_HHMMSS.csv`

**Processing Responsibilities:**
- Listen to all CAN bus traffic (promiscuous mode)
- Parse CAN messages and convert to CSV format
- Write to microSD with buffering (reduce write cycles)
- Detect flight start/stop (engine RPM threshold)
- Compress old flight logs (gzip) to save space
- Provide WiFi access point for log retrieval
- Safe shutdown on button press (unmount SD card properly)
- Monitor SD card health (bad blocks, wear leveling)
- Sync time with GPS via CAN messages

**CAN Bus Messages (Transmitted):**

| Message ID | Name | Frequency | Data Layout |
|------------|------|-----------|-------------|
| 0x300 | Logger Status | 1Hz | Byte 0: Status (logging/idle/fault), 1: SD free % (0-100), 2-3: Files count |
| 0x30F | Heartbeat | 1Hz | Byte 0: Node ID (0x03), 1: Uptime (minutes), 2-7: Reserved |

**CAN Bus Messages (Received):**
- **All messages from 0x100-0x2FF** (Engine + Instruments)
- Log all received messages with timestamps

**Power Budget:**
- Idle: 150mA @ 5V (750mW)
- Active logging: 200mA @ 5V (1000mW)
- SD write burst: 250mA @ 5V (1250mW, <1 second)
- WiFi active: 250mA @ 5V (1250mW)

**Software Architecture:**
- **Operating System:** Raspberry Pi OS Lite (no desktop)
- **Programming:** Python 3 for logging daemon
- **CAN Interface:** python-can library with MCP2515 driver
- **File I/O:** CSV writer with 10-second write buffering
- **Web Interface:** Flask app for log browsing/download
- **Startup:** systemd service for logging daemon (auto-start)

**Key Software Components:**
```python
# Logging daemon structure
class CANLogger:
    def __init__(self):
        self.bus = can.interface.Bus(bustype='socketcan', channel='can0')
        self.csv_writer = CSVWriter('/media/sdcard/logs/')
        self.buffer = []

    def listen(self):
        # Receive CAN messages
        message = self.bus.recv(timeout=1.0)
        if message:
            self.buffer.append(self.parse_message(message))

    def flush(self):
        # Write buffer to CSV every 10 seconds
        if len(self.buffer) > 0:
            self.csv_writer.write_rows(self.buffer)
            self.buffer = []
```

**File System Layout:**
```
/media/sdcard/
├── logs/
│   ├── flight_20250118_093042.csv
│   ├── flight_20250118_110523.csv
│   └── compressed/
│       └── flight_20250117_083015.csv.gz
├── config/
│   └── logger_config.json
└── status/
    └── health_log.txt
```

**WiFi Access:**
- **SSID:** N2-Logger-XXXX (last 4 of MAC)
- **Password:** Configured in /etc/hostapd/hostapd.conf
- **IP Address:** 192.168.42.1
- **Web Interface:** http://192.168.42.1:5000
- **Features:** Browse logs, download CSV, view system status, clear old logs

**Memory Budget:**
- OS footprint: ~180MB RAM (minimal install)
- Logging daemon: ~30MB RAM
- CAN buffers: ~5MB RAM
- Web server: ~20MB RAM (when active)
- Available: ~277MB RAM for buffers and caching

**Safe Shutdown Procedure:**
1. Detect button press (GPIO interrupt)
2. Flush all buffered data to SD card
3. Close CSV file properly
4. Sync filesystem (`sync` command)
5. Unmount SD card (`umount /media/sdcard`)
6. Shutdown system (`shutdown -h now`)

---

### Subsystem 4: Pilot Information System

**Hardware Platform:** Raspberry Pi 4 Model B (2GB RAM)

**Key Components:**
- BCM2711 SoC (4× ARM Cortex-A72 @ 1.5GHz)
- 2GB LPDDR4 RAM
- Official 7" touchscreen display (800×480, DSI interface)
- MCP2515 CAN controller (SPI) or USB-CAN adapter
- USB keyboard/mouse (for configuration)
- HDMI output (alternative to touchscreen)
- Quad USB ports for peripherals
- Power button with hardware shutdown circuit

**Display Strategy:**
- **Multi-page GUI:** 8+ screens accessible via swipe/touch
- **Real-time graphs:** Line charts for trends (EGT, CHT, altitude)
- **Configurable layout:** User can customize screen content
- **Alert overlay:** Critical warnings appear over all screens
- **Night mode:** Red backlight, reduced brightness for night flying

**Processing Responsibilities:**
- Receive all CAN bus data (Engine + Instruments)
- Render real-time graphical displays
- Historical trending (last 60 minutes of data in RAM)
- Configuration interface for system settings
- QNH/altimeter setting with numeric keypad
- Date/time configuration (sync to GPS or manual)
- Alert/warning management (acknowledge, silence, log)
- Export data to USB drive (flight logs, configuration)

**CAN Bus Messages (Transmitted):**

| Message ID | Name | Frequency | Data Layout |
|------------|------|-----------|-------------|
| 0x400 | System Config | On change | Byte 0-1: QNH (hPa×10), 2: Units (0=metric, 1=imperial), 3-7: Reserved |
| 0x401 | Time Sync | 1/minute | Byte 0-3: Unix timestamp (seconds since epoch) |
| 0x402 | Alert Acknowledge | On event | Byte 0: Alert ID being acknowledged, 1-7: Reserved |
| 0x40F | Heartbeat | 1Hz | Byte 0: Node ID (0x04), 1: Uptime (minutes), 2-7: Reserved |

**CAN Bus Messages (Received):**
- **All messages from 0x100-0x3FF** (Engine + Instruments + Logger status)

**Power Budget:**
- Idle (display off): 400mA @ 5V (2W)
- Active display: 1000mA @ 5V (5W)
- Peak (touchscreen + processing): 1100mA @ 5V (5.5W)

**Software Architecture:**
- **Operating System:** Raspberry Pi OS with desktop (for GUI)
- **Programming:** Python 3 + Qt5 (PyQt5 framework)
- **CAN Interface:** python-can library
- **GUI Framework:** Qt5 for cross-platform GUI
- **Graphing:** pyqtgraph for real-time plotting
- **Configuration:** JSON files for user preferences

**Screen Layout Examples:**

**Screen 1: Engine Overview**
```
┌─────────────────────────────────────────────┐
│ Engine Monitor          14:32:15   2450 RPM │
├─────────────────────────────────────────────┤
│ EGT 1: 685°C  CHT 1: 195°C   Oil: 95°C     │
│ EGT 2: 690°C  CHT 2: 198°C   Pres: 55 PSI  │
│ EGT 3: 688°C  CHT 3: 196°C   Hours: 124.3  │
│ EGT 4: 692°C  CHT 4: 199°C   MP: 25.2 inHg │
│                                              │
│ [Bar graph showing EGT/CHT across cylinders]│
│                                              │
│ ⚠ CHT 2 approaching warning limit           │
└─────────────────────────────────────────────┘
```

**Screen 2: Flight Instruments**
```
┌─────────────────────────────────────────────┐
│ Flight Instruments               5,420 ft   │
├─────────────────────────────────────────────┤
│                                              │
│     [Attitude indicator]  [Heading indicator]│
│      Pitch: +5°            HDG: 285°        │
│      Roll: -2°             MAG deviation    │
│                                              │
│  Alt: 5,420 ft   VS: +250 ft/min           │
│  QNH: 1013 hPa   GPS Alt: 5,445 ft         │
│                                              │
│  Fuel: 45%       Battery: 13.8V / 5.2A     │
└─────────────────────────────────────────────┘
```

**Screen 3: Navigation**
```
┌─────────────────────────────────────────────┐
│ Navigation                      GS: 95 kt   │
├─────────────────────────────────────────────┤
│                                              │
│  Position: 52.5235°N, 13.4115°E            │
│  Ground Speed: 95 knots                     │
│  Track: 285°                                │
│  GPS Satellites: 12 (Good)                  │
│                                              │
│  [Map display would go here in future]      │
│                                              │
│  Time: 14:32:15 UTC                         │
└─────────────────────────────────────────────┘
```

**Screen 4: Trending (EGT/CHT)**
```
┌─────────────────────────────────────────────┐
│ Temperature Trends (Last 60 min)            │
├─────────────────────────────────────────────┤
│                                              │
│  [Line graph: 4 EGT traces over time]       │
│  [Line graph: 4 CHT traces over time]       │
│                                              │
│  Markers for takeoff, cruise, descent       │
│  Zoom: [15min] [30min] [60min]              │
│                                              │
└─────────────────────────────────────────────┘
```

**Configuration Screens:**
- QNH setting (numeric keypad, current altimeter pressure)
- Date/Time (sync to GPS or manual set)
- Units (metric/imperial, °C/°F, hPa/inHg)
- Alert limits (EGT/CHT warn/critical thresholds)
- Display brightness (0-100%, auto/manual)
- Network settings (WiFi credentials for logger access)

**Memory Budget:**
- OS + GUI: ~400MB RAM
- Application: ~200MB RAM
- Historical data: ~100MB RAM (60 min @ 10Hz = 360K samples)
- CAN buffers: ~10MB RAM
- Available: ~1.3GB RAM for future expansion

---

## CAN Bus Protocol Design

### Message ID Allocation Strategy

**ID Range Allocation:**
- **0x100-0x1FF:** Engine Monitor System (256 IDs)
- **0x200-0x2FF:** Instrument System (256 IDs)
- **0x300-0x3FF:** Data Logger System (256 IDs)
- **0x400-0x4FF:** Pilot Information System (256 IDs)
- **0x500-0x5FF:** Reserved for future expansion
- **0x7F0-0x7FF:** Emergency/diagnostic messages

**Priority Scheme:**
- Lower CAN ID = Higher priority (CAN arbitration rule)
- **Critical messages:** 0x100-0x10F (engine protection data)
- **Important messages:** 0x110-0x1FF, 0x200-0x2FF (flight data)
- **Status messages:** 0x300-0x3FF (logging, non-critical)
- **Configuration:** 0x400-0x4FF (can be delayed)

### Message Format Specifications

**Standard CAN Frame:**
- 11-bit identifier (standard CAN 2.0B)
- 0-8 data bytes
- CRC built into CAN protocol (automatic)

**Byte Order:** Little-endian (LSB first)

**Temperature Encoding:**
- 16-bit signed integer, °C × 10 (range: -3276.8°C to +3276.7°C)
- Example: 195.5°C → 1955 (0x07A3) → Byte 0: 0xA3, Byte 1: 0x07

**Pressure Encoding:**
- 16-bit unsigned integer, hPa × 10 (range: 0 to 6553.5 hPa)
- Example: 1013.2 hPa → 10132 (0x2794) → Byte 0: 0x94, Byte 1: 0x27

**GPS Coordinate Encoding:**
- 32-bit signed integer, degrees × 1,000,000
- Example: 52.5235°N → 52523500 (0x03215F9C)

**Status Flags (Byte):**
```
Bit 7: Critical alert active
Bit 6: Warning alert active
Bit 5: Sensor fault detected
Bit 4: GPS fix valid
Bit 3: IMU calibrated
Bit 2: Logging active
Bit 1: Reserved
Bit 0: System armed/active
```

### Update Rate Requirements

| Message Type | Required Rate | Justification |
|--------------|---------------|---------------|
| EGT/CHT | 10Hz (100ms) | Engine protection, fast response needed |
| Oil Temp/Pressure | 10Hz | Engine monitoring, safety-critical |
| Tachometer | 10Hz | Real-time engine speed display |
| Altitude/Pressure | 5Hz (200ms) | Flight instruments, adequate for display |
| Attitude (pitch/roll) | 5Hz | Smooth attitude display |
| GPS Position | 1Hz (1s) | Sufficient for navigation at typical speeds |
| Fuel/Battery | 2Hz (500ms) | Slowly-changing values |
| Heartbeat | 1Hz | Fault detection within 3 seconds acceptable |
| Configuration | On change | Transmitted only when user changes setting |

**Bus Utilization Calculation:**
- CAN frame overhead: 47 bits (preamble, ID, CRC, ACK, etc.)
- 8-byte data payload: 64 bits
- Total per frame: 111 bits @ 500 kbps = 222 µs per frame

**Worst case bus load:**
- Engine (10Hz): 6 messages × 10Hz = 60 msg/s
- Instruments (5Hz): 5 messages × 5Hz = 25 msg/s
- Instruments (1-2Hz): 3 messages × 1.5Hz = 4.5 msg/s
- Logger/Pilot (1Hz): 4 messages × 1Hz = 4 msg/s
- **Total: 93.5 messages/second**
- **Bus time: 93.5 × 222 µs = 20.7 ms/second = 2.1% utilization**
- **Excellent margin for error handling, retransmissions, and expansion**

### Error Handling Approach

**CAN Protocol Built-in Error Detection:**
- CRC check on every frame (automatic)
- Bit stuffing error detection
- Automatic retransmission on error (up to 15 retries)

**Application-Level Error Handling:**

**1. Timeout Detection**
- Each receiver tracks "last received time" for each message ID
- If no message for 3× expected interval, flag as timeout
- Display shows "---" or "FAULT" for timed-out data
- Example: EGT messages expected every 100ms, timeout after 300ms

**2. Range Validation**
- Receiving node checks if value is within physically possible range
- Example: EGT must be 0-1000°C, reject if outside
- Invalid data triggers fault flag, previous value held

**3. Heartbeat Monitoring**
- Each node transmits heartbeat at 1Hz
- If no heartbeat for 3 seconds, node assumed offline
- Pilot Info displays warning: "ENGINE MONITOR OFFLINE"
- System continues with last known good data

**4. Sensor Fault Propagation**
- If sensor fails (e.g., MAX6675 returns error), flag in status byte
- CAN message still sent, but with fault flag set
- Receiving systems can distinguish "no data" from "sensor failed"

**5. Error Logging**
- Data Logger records all CAN errors (timeouts, invalid data, offline nodes)
- Timestamp + error code + context saved to CSV
- Post-flight analysis can identify intermittent issues

---

## Migration Strategy

### Reusable Code and Patterns

**From Current System to New Subsystems:**

**1. Event-Driven Architecture (100% reusable)**
- EventManager library works on Arduino Nano, Mega, Teensy
- `handleEvents()` → `raiseEvents()` pattern proven effective
- Fixed-size queues (8 events) and listeners (8) appropriate for new systems
- **Action:** Copy EventManager library to all Arduino subsystems unchanged

**2. MAX6675 Sensor Reading (95% reusable)**
- Current code reads 4× MAX6675 sensors via SPI
- Engine Monitor needs 9× MAX6675 sensors (same interface, more CS pins)
- Existing pattern: `tempEGT.readCelsius()` → store in `SensorData[]`
- **Action:** Extend to support 9 CS pins, same reading logic

**3. Barometer Integration (90% reusable)**
- Current BMP085 code structure applies to BMP390 (both I2C)
- QNH calibration logic transfers directly
- Altitude calculation identical (standard atmosphere model)
- **Action:** Update library to BMP390 (better accuracy), keep API same

**4. Clock/RTC Handling (100% reusable)**
- Current DS3231/PCF8523 RTC code works on all Arduino systems
- Engine hours tracking uses same time-keeping approach
- **Action:** Copy clock library to Engine Monitor for hours tracking

**5. Memory Management Patterns (100% reusable)**
- `F()` macro for PROGMEM strings proven effective
- sprintf() to char buffers avoids String object overhead
- Fixed-size arrays for sensor data efficient
- **Action:** Apply same patterns in all new Arduino code

**6. Button/Rotary Control (not directly reusable)**
- Control logic stays only on Pilot Info system (GUI replaces physical controls)
- Event-driven pattern applicable to GUI events (button clicks, swipes)
- **Action:** Port control event concepts to Qt GUI signals/slots

**7. Virtual Screen System (obsolete)**
- No longer needed with graphical display on RPi 4
- Concept of multiple "screens" translates to GUI pages
- **Action:** Implement GUI pages in Qt with similar navigation concept

### What Needs Redesign

**1. Display System**
- **Current:** 16×2 character LCD, virtual screens, rotary navigation
- **New:** 7" touchscreen GUI with graphical pages
- **Redesign:** Complete GUI rewrite in Qt/Python
- **Rationale:** Graphical display enables trends, graphs, intuitive configuration

**2. Communication Layer**
- **Current:** All data in-process (function calls)
- **New:** CAN bus messaging between subsystems
- **Redesign:** CAN driver integration, message parsing, timeout handling
- **Rationale:** Distributed system requires robust inter-node communication

**3. Data Storage**
- **Current:** 512 bytes EEPROM for configuration
- **New:** MicroSD card with gigabytes of log storage
- **Redesign:** File-based logging, CSV format, log rotation
- **Rationale:** Flight data logging requires persistent, large-capacity storage

**4. Configuration Management**
- **Current:** Hardcoded values, minimal runtime config
- **New:** JSON config files, GUI-based configuration
- **Redesign:** Configuration sync via CAN, persistent storage
- **Rationale:** Multi-node system needs coordinated configuration

**5. Error Handling**
- **Current:** Minimal fault detection (sensor active() checks)
- **New:** Comprehensive fault detection, logging, recovery
- **Redesign:** Watchdog timers, heartbeat monitoring, fault flags
- **Rationale:** Safety-critical system requires robust error handling

### Testing Strategy During Migration

**Phase 1: Parallel Development (Months 1-2)**
- Keep existing single-board system operational as reference
- Develop Engine Monitor and Instrument System in parallel
- Test each subsystem standalone with simulated CAN data
- Validate sensor readings against current system

**Phase 2: CAN Bus Integration (Month 3)**
- Connect Engine Monitor + Instrument System on CAN bus
- Implement Data Logger (records all CAN traffic)
- Verify message timing and bus utilization
- Compare logged data against expectations

**Phase 3: GUI Development (Month 4)**
- Develop Pilot Info system GUI (can use logged CAN data for testing)
- Implement all display screens, configuration interface
- Test with recorded flight data from Phase 2
- Validate graphical trending and alert logic

**Phase 4: System Integration (Month 5)**
- Connect all four subsystems on bench test setup
- Simulate full flight profile (startup → cruise → shutdown)
- Verify end-to-end data flow (sensors → CAN → display → log)
- Stress test: Sensor failures, node crashes, CAN errors

**Phase 5: Field Testing (Month 6)**
- Install in aircraft (ground testing initially)
- Engine ground runs (verify RPM, temperatures, pressures)
- Taxi tests (verify GPS, attitude, logging)
- Flight testing (start with short flights, build confidence)
- Post-flight data analysis (review logs, identify anomalies)

**Test Equipment Needed:**
- CAN bus analyzer (e.g., CANable USB adapter, ~$40)
- Oscilloscope (verify CAN signal integrity)
- Temperature simulator (resistors to simulate thermocouples)
- Pressure simulator (adjustable voltage sources for analog sensors)
- GPS simulator (software-defined radio or pre-recorded NMEA)

**Test Cases:**

| Test ID | Description | Pass Criteria |
|---------|-------------|---------------|
| T-001 | Engine Monitor reads all 9 temperatures | All sensors read within ±2°C of known value |
| T-002 | CAN bus message timing | All messages transmitted at required rate ±10% |
| T-003 | Data Logger records 1-hour session | CSV file created, all messages logged, no corruption |
| T-004 | Pilot Info displays engine data | Display updates within 500ms of CAN message |
| T-005 | Alert triggering | Critical alert displayed within 200ms of threshold |
| T-006 | Node failure detection | Offline node detected within 3 seconds |
| T-007 | Configuration sync | QNH change on Pilot Info reflected on Instruments within 2s |
| T-008 | Engine hours tracking | Accumulated time matches stopwatch ±1 second |
| T-009 | GPS fix acquisition | Cold start fix <60s, warm start <30s |
| T-010 | Sensor failure handling | Failed sensor flagged, system continues operation |

---

## Development Roadmap (Next 3-6 Months)

### Phase 1: Foundation and Critical Systems (Months 1-2)

**Month 1: Engine Monitor System**

**Week 1-2: Hardware Setup**
- Procure components: Arduino Nano/Uno, MCP2515, 9× MAX6675, sensors
- Assemble on breadboard/prototype shield
- Verify all sensor connections (SPI CS lines, analog inputs, tachometer input)
- **Deliverable:** Functioning hardware prototype, all sensors readable

**Week 3-4: Software Development**
- Port EventManager library and MAX6675 reading code from current system
- Implement CAN bus integration (mcp_can library)
- Develop tachometer frequency measurement (Timer1 or interrupt-based)
- Implement engine hours tracking with EEPROM persistence
- Create CAN message encoding functions
- **Deliverable:** Engine Monitor transmitting CAN messages at 10Hz

**Testing Milestones:**
- All 9 temperature sensors read correctly (±1°C accuracy)
- Tachometer reads 0-6000 RPM (test with function generator)
- CAN messages verified with CAN analyzer
- Engine hours persist across power cycles
- **Risk:** MAX6675 noise issues on long SPI bus - mitigation: shorter wires, 100nF caps

**Month 2: Instrument System**

**Week 1-2: Hardware Setup**
- Procure components: Arduino Mega or Teensy 4.0, MCP2515, BMP390, BMI088, GPS
- Assemble multi-sensor setup (pay attention to I2C address conflicts)
- Verify GPS NMEA output (9600 baud UART)
- Test IMU and magnetometer calibration
- **Deliverable:** Functioning instrument hardware, all sensors initialized

**Week 3-4: Software Development**
- Port Barometer library (upgrade BMP085 → BMP390)
- Implement GPS NMEA parser (focus on RMC and GGA sentences)
- Integrate IMU sensor fusion (Madgwick filter or complementary filter)
- Implement CAN bus integration (parallel to Engine Monitor approach)
- Create altitude calculation with QNH support
- **Deliverable:** Instrument System transmitting CAN messages at 5Hz

**Testing Milestones:**
- Altitude calculation within ±10 ft of known elevation
- GPS fix acquired outdoors (<60s cold start)
- IMU pitch/roll accurate within ±2° (compare to level surface)
- Heading accurate within ±5° (compare to compass)
- CAN messages verified with analyzer
- **Risk:** IMU sensor fusion complexity - mitigation: use proven library (Adafruit AHRS)

### Phase 2: Data Persistence and Communication (Month 3)

**Week 1-2: Data Logger System**
- Set up Raspberry Pi Zero 2 W with Raspberry Pi OS Lite
- Configure MCP2515 CAN interface (enable SPI, load kernel module)
- Develop Python logging daemon using python-can library
- Implement CSV writer with 10-second buffering
- Test microSD write performance and wear leveling
- **Deliverable:** Data Logger recording all CAN traffic to CSV

**Week 3: CAN Bus Integration Testing**
- Connect Engine Monitor + Instrument System + Data Logger on CAN bus
- Verify bus termination (120Ω resistors)
- Measure CAN bus utilization with analyzer (should be <5%)
- Run 1-hour logging test (verify no message loss)
- Analyze CSV output for timing accuracy
- **Deliverable:** Stable 3-node CAN network with data logging

**Week 4: WiFi and Safe Shutdown**
- Configure WiFi access point on Data Logger
- Develop Flask web interface for log browsing/download
- Implement safe shutdown button (GPIO + systemd integration)
- Test power-loss scenarios (ensure no SD card corruption)
- **Deliverable:** WiFi-accessible data logger with safe shutdown

**Testing Milestones:**
- 1-hour continuous logging with zero message loss
- CAN bus timing verified (<10% jitter)
- SD card survives 100 power cycles without corruption
- WiFi range covers typical cockpit area (10+ feet)
- **Risk:** SD card corruption on power loss - mitigation: aggressive sync, journaling filesystem

### Phase 3: User Interface and Configuration (Month 4)

**Week 1-2: Pilot Information Hardware**
- Set up Raspberry Pi 4 with 7" touchscreen
- Configure MCP2515 CAN interface (or USB-CAN adapter)
- Install Qt5 development environment (PyQt5)
- Test touchscreen calibration and responsiveness
- **Deliverable:** RPi 4 with functional touchscreen, receiving CAN data

**Week 3-4: GUI Development**
- Design and implement main GUI pages (Engine, Instruments, Navigation, Trending)
- Implement real-time graphing (pyqtgraph for EGT/CHT trends)
- Create configuration screens (QNH, date/time, units, alerts)
- Implement alert overlay system (critical warnings)
- Develop CAN message transmission for configuration sync
- **Deliverable:** Functional GUI with all screens, configuration interface

**Testing Milestones:**
- GUI updates within 500ms of CAN message reception
- Trending graphs smooth at 10Hz data rate
- Configuration changes transmitted on CAN within 1 second
- Touchscreen responsive (button press within 100ms)
- Night mode reduces brightness, changes colors to red
- **Risk:** Qt performance on RPi 4 - mitigation: optimize rendering, use OpenGL acceleration

### Phase 4: System Integration and Validation (Month 5)

**Week 1: Bench Integration**
- Assemble all four subsystems on bench setup
- Verify CAN bus wiring (linear topology, termination resistors)
- Power on all systems simultaneously (check for startup sequence issues)
- Verify heartbeat messages from all nodes
- **Deliverable:** Complete 4-node system operational on bench

**Week 2: End-to-End Testing**
- Simulate full flight profile with sensor stimulation
- Startup: Engine crank (RPM ramps up), temperatures rise
- Cruise: Steady-state RPM, temperatures, altitude changes
- Shutdown: RPM drops, temperatures cool, engine hours incremented
- Verify data logged correctly, GUI displays accurate, no alerts missed
- **Deliverable:** Successful simulated flight, end-to-end data validated

**Week 3: Fault Injection Testing**
- Disconnect sensors one at a time (verify fault detection)
- Kill CAN nodes (verify timeout detection within 3 seconds)
- Corrupt CAN messages (verify range checking rejects invalid data)
- Power cycle individual nodes (verify recovery and reconnection)
- Overload CAN bus (transmit extra messages, verify priority handling)
- **Deliverable:** Fault handling validated, system robust to failures

**Week 4: Documentation and Training**
- Create installation guide (wiring diagrams, pin assignments)
- Write user manual (GUI operation, configuration procedures)
- Document troubleshooting procedures (LED codes, fault diagnosis)
- Create maintenance checklist (pre-flight checks, periodic calibration)
- **Deliverable:** Complete documentation package

**Testing Milestones:**
- All fault scenarios detected and logged
- System recovers gracefully from single-node failure
- Data logging continues during fault conditions
- GUI displays appropriate warnings for offline nodes
- **Risk:** Undiscovered edge cases - mitigation: extensive fault matrix testing

### Phase 5: Field Testing and Iteration (Month 6)

**Week 1-2: Ground Testing**
- Install in aircraft panel (avoid flight-critical systems initially)
- Connect to aircraft power (verify voltage regulation under alternator load)
- Engine ground run: Monitor temperatures, RPM, pressures
- Compare readings to factory gauges (validate calibration)
- Test alert thresholds (intentionally exceed limits during ground run)
- **Deliverable:** System operational in aircraft during ground runs

**Week 3: Taxi Testing**
- Perform taxi tests (GPS fix, attitude sensor, vibration exposure)
- Verify GPS tracks aircraft movement on ground
- Check for sensor noise from engine vibration
- Test touchscreen usability in cockpit environment
- Download and analyze logged data after taxi test
- **Deliverable:** System validated during taxi operations

**Week 4: Flight Testing**
- Conduct short flight (local pattern, 30-60 minutes)
- Monitor system operation throughout flight (takeoff, cruise, landing)
- Post-flight data analysis (review trends, identify anomalies)
- Iterate on alert thresholds based on actual flight data
- Conduct longer flight (cross-country, 2+ hours)
- **Deliverable:** System validated in actual flight operations

**Testing Milestones:**
- Zero false alerts during normal operations
- Data logging complete for entire flight (no gaps)
- GPS tracking accurate (within 10m of known position)
- Attitude display matches visual reference (horizon)
- Engine parameters match pilot observations
- **Risk:** Unexpected behavior in flight - mitigation: conservative test plan, monitor with backup gauges

**Success Criteria for Phase 5:**
- 5+ successful flights with zero system-caused issues
- All sensor readings validated against known-good references
- Pilot reports system intuitive and trustworthy
- Data logs complete and analyzable
- No safety concerns identified

---

## Technical Recommendations

### Development Tools and Practices

**Version Control:**
- Git repository for all code (Arduino C++, Python, Qt GUI)
- Separate branches for each subsystem during development
- Tag releases (v1.0 for initial flight testing)
- GitHub/GitLab for collaboration and issue tracking

**Arduino Development:**
- PlatformIO (current system uses this, maintain consistency)
- Upload via USB serial (avoid ISP unless bootloader repair needed)
- Serial debugging at 115200 baud (faster than current 9600 for CAN traffic logs)
- Unit tests for critical functions (altitude calc, CAN parsing)

**Raspberry Pi Development:**
- Cross-compile on x86 Linux (faster than native compilation on RPi)
- systemd services for auto-start of logging/GUI daemons
- journalctl for log viewing (systemd's logging system)
- Python virtual environments (isolate dependencies)

**CAN Bus Tools:**
- CANable USB adapter (~$40) for bus monitoring and debugging
- can-utils (Linux command-line tools: candump, cansend, cangen)
- Wireshark with SocketCAN plugin (visualize CAN traffic)
- PCAN-View or Cangaroo (Windows GUI for CAN analysis)

**Simulation and Testing:**
- Flight data simulator (Python script that replays recorded CAN messages)
- Hardware-in-the-loop testing (real sensors, simulated engine behavior)
- Automated test scripts (pytest for Python, Unity for Arduino C)

### Testing Approach

**Unit Testing:**
- Test individual functions in isolation
- Mock CAN bus interfaces (return canned data)
- Validate sensor reading algorithms (known input → expected output)
- Test configuration parsing (JSON, EEPROM)

**Integration Testing:**
- Test two subsystems together (e.g., Engine Monitor + Data Logger)
- Verify CAN communication layer (timing, error handling)
- Test configuration sync (change on Pilot Info → reflected on Instruments)

**System Testing:**
- All four subsystems connected
- Simulated flight profile (automated stimulus)
- Long-duration tests (8+ hours continuous operation)
- Stress testing (rapid sensor changes, high CAN bus load)

**Field Testing:**
- Ground runs (controlled environment, easy to abort)
- Taxi tests (low-risk, validate GPS and vibration exposure)
- Flight tests (progressive: short local flights → long cross-country)
- Post-flight data analysis (review logs for anomalies)

**Regression Testing:**
- Maintain test suite of recorded CAN data from previous flights
- Run simulator against historical data after code changes
- Verify no changes in behavior (alerts, calculations)

### Code Organization and Reuse Strategy

**Directory Structure:**
```
n2-arduino/
├── subsystems/
│   ├── engine_monitor/
│   │   ├── src/
│   │   ├── lib/          (EventManager, MAX6675, CAN)
│   │   ├── test/
│   │   └── platformio.ini
│   ├── instrument_system/
│   │   ├── src/
│   │   ├── lib/          (EventManager, BMP390, IMU, GPS, CAN)
│   │   ├── test/
│   │   └── platformio.ini
│   ├── data_logger/
│   │   ├── logger_daemon.py
│   │   ├── web_interface.py
│   │   ├── systemd/      (service files)
│   │   └── config/
│   └── pilot_info/
│       ├── gui_main.py
│       ├── screens/      (Qt UI files)
│       ├── can_interface.py
│       └── config/
├── shared/
│   ├── can_protocol.h    (CAN message definitions, shared by all)
│   ├── sensor_types.h    (reused from current system)
│   └── eeprom_map.h      (reused from current system)
├── tools/
│   ├── can_simulator.py  (replay recorded CAN data)
│   ├── sensor_simulator/ (Arduino sketch for sensor simulation)
│   └── flash_all.sh      (script to upload to all Arduino boards)
├── docs/
│   ├── Architecture/     (this document)
│   ├── CAN_Protocol.md   (detailed message specs)
│   ├── Installation.md   (wiring diagrams, setup)
│   └── User_Manual.md    (pilot-facing documentation)
└── test_data/
    ├── flight_20250101_120000.csv
    └── simulated_profiles/ (canned test data)
```

**Shared Code:**
- `shared/can_protocol.h`: CAN message IDs, data layouts (included by all subsystems)
- `shared/sensor_types.h`: Sensor type constants (reused from current system)
- EventManager library (copied to each Arduino subsystem, versioned)
- Avoid "shared library" complications (Arduino build system limitations)

**Code Style:**
- Arduino: Follow current system's style (camelCase, F() for strings, sprintf())
- Python: PEP 8 style guide (snake_case, type hints, docstrings)
- Comments: Explain *why*, not *what* (code should be self-documenting)
- Function length: <50 lines (if longer, refactor)

### Documentation Requirements

**Code Documentation:**
- Header comments for all files (author, date, purpose)
- Function comments (parameters, return values, side effects)
- Complex algorithms explained (sensor fusion, altitude calculation)
- CAN message encoding/decoding documented inline

**System Documentation:**
- **Architecture diagram** (block diagram of subsystems, already in this document)
- **CAN protocol specification** (message IDs, formats, timing - expand from this doc)
- **Wiring diagrams** (pin assignments, power distribution, CAN bus topology)
- **Calibration procedures** (QNH setting, fuel level calibration, tachometer scaling)

**User Documentation:**
- **Installation guide** (step-by-step with photos)
- **User manual** (GUI operation, configuration screens, alert meanings)
- **Troubleshooting guide** (common issues, LED codes, reset procedures)
- **Maintenance checklist** (pre-flight checks, periodic calibration, software updates)

**Development Documentation:**
- **Build instructions** (how to compile and upload each subsystem)
- **Testing procedures** (how to run unit tests, integration tests)
- **Simulator usage** (how to use CAN simulator for testing)
- **Contribution guide** (for future developers joining project)

---

## Risk Assessment

### Technical Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| CAN bus noise/errors | Medium | High | Proper termination, shielded cable, test extensively |
| SD card corruption | Medium | Medium | Journaling filesystem, sync frequently, safe shutdown |
| MAX6675 SPI contention | Low | Medium | Separate CS lines, SPI speed <1MHz, add delays |
| IMU sensor drift | Medium | Low | Periodic recalibration, sensor fusion with GPS/mag |
| RPi boot time (>30s) | High | Low | Acceptable for non-critical display system |
| GUI performance lag | Low | Medium | Optimize rendering, use OpenGL, reduce update rate |
| Power supply noise | Medium | High | Separate regulators, filtering caps, isolated grounds |
| GPS fix loss | Low | Low | Expected in some environments, display warning |
| Node crash/hang | Low | High | Watchdog timers, heartbeat monitoring, auto-reset |
| Flash memory wear | Low | Low | EEPROM write limits respected (>100K cycles) |

### Schedule Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Component delays | Medium | Medium | Order early, have backup suppliers, plan buffer time |
| Learning curve (CAN) | Low | Low | Prototype early, use proven libraries (mcp_can) |
| Scope creep | High | High | Strict feature freeze after Phase 3, track in issues |
| Testing delays | Medium | High | Start testing early, automate where possible |
| Weather (flight tests) | High | Low | Flexible schedule, ground testing first |

### Safety Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| False critical alert | Low | High | Conservative thresholds, alert testing, acknowledge feature |
| Missed real alert | Very Low | Critical | Redundant display (LED on Engine Monitor), audio alerts |
| Display failure | Low | Medium | System continues operating, backup gauges required |
| Sensor failure | Medium | Medium | Fault detection, display warning, continue with remaining sensors |
| Electrical fire | Very Low | Critical | Proper wire sizing, fusing, thermal testing, no exposed conductors |

**Safety Philosophy:**
- This system is **supplemental instrumentation**, not primary flight instruments
- Pilots must maintain situational awareness using standard gauges
- System designed to fail safely (warnings displayed, operation continues)
- Extensive ground and flight testing before reliance in critical situations

---

## Dependencies and Critical Path

### Hardware Dependencies
- Arduino boards (2-4 week delivery if out of stock)
- MCP2515 CAN controllers (4× required, 1-2 week delivery)
- MAX6675 thermocouple modules (9× required, 2-3 week delivery from China)
- Raspberry Pi 4 + touchscreen (2-4 week delivery, supply chain issues)
- MicroSD cards (readily available)
- Sensors (BMP390, IMU, GPS) (1-2 week delivery)

**Critical Path:** Raspberry Pi 4 + touchscreen (longest lead time) - order immediately

### Software Dependencies
- PlatformIO (already in use, no change)
- Arduino libraries: EventManager (current), mcp_can (new), Adafruit sensors (new)
- Python libraries: python-can, pyqtgraph, Flask (installable via pip)
- Raspberry Pi OS (downloadable, 1-2 hour setup per board)

**Critical Path:** None (all software readily available)

### Knowledge Dependencies
- CAN bus protocol understanding (1 week learning curve)
- Qt/PyQt5 GUI development (2 week learning curve for basic functionality)
- IMU sensor fusion algorithms (1 week to integrate existing library)
- Raspberry Pi systemd service management (2 days learning curve)

**Critical Path:** Qt/PyQt5 GUI development (needed for Phase 3, can learn during Phase 1-2)

### Testing Dependencies
- CAN bus analyzer (order in Month 1, 1 week delivery)
- Aircraft availability for ground/flight testing (coordinate with owner)
- Weather suitable for flight testing (can't control, buffer time needed)

**Critical Path:** Aircraft availability (coordinate early, book test slots)

### Overall Critical Path

**Longest dependency chain:**
1. Order RPi 4 + touchscreen (Day 1) → 4 weeks delivery → GUI development starts (Week 5)
2. Develop Engine Monitor + Instruments (Weeks 1-8) → CAN integration (Week 9)
3. CAN integration complete (Week 12) → GUI integration (Weeks 13-16)
4. System integration (Week 17-20) → Flight testing (Weeks 21-24)

**Total critical path: 24 weeks (6 months)**

**Parallel activities:**
- Data Logger development (Month 3) can overlap with Engine Monitor/Instruments (Months 1-2)
- GUI development (Month 4) can start earlier if RPi arrives sooner
- Documentation can be written concurrently with development

---

## Conclusion

This multi-system architecture evolution preserves the proven strengths of the current N2-Arduino system while addressing scalability, real-time performance, and safety requirements for a comprehensive aircraft instrumentation system. The distributed approach with CAN bus interconnection provides:

- **Specialization:** Each subsystem optimized for its specific role
- **Fault Isolation:** Failures contained to individual nodes
- **Scalability:** Easy to add new sensors or subsystems in the future
- **Maintainability:** Modular design simplifies troubleshooting and upgrades
- **Safety:** Redundant data logging and robust error handling

The 6-month development roadmap is realistic and achievable, with well-defined milestones and deliverables. The migration strategy preserves valuable existing code (event system, sensor reading patterns, memory management) while introducing new capabilities (graphical display, data logging, distributed communication).

**Key Success Factors:**
- Early hardware procurement (especially RPi 4 + touchscreen)
- Rigorous testing at each phase (don't skip integration testing)
- Conservative flight test approach (ground runs → taxi → short flights)
- Comprehensive documentation (future maintainers will thank you)
- Realistic timeline (6 months assumes part-time work, no major setbacks)

**Next Immediate Actions:**
1. Review and approve this plan with project stakeholders
2. Order long-lead-time components (RPi 4, touchscreen, MCP2515, MAX6675)
3. Set up development environment (PlatformIO, Python, Qt)
4. Begin Month 1 activities (Engine Monitor hardware assembly)
5. Schedule aircraft availability for ground/flight testing (Months 5-6)

This architecture provides a solid foundation for a professional-grade aircraft instrumentation system that can evolve with future requirements (EFIS integration, autopilot interface, wireless telemetry) while maintaining safety and reliability.

---

**Document Control**

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2025-01-18 | Embedded Systems Team | Initial comprehensive planning document |

**Approvals**

| Role | Name | Signature | Date |
|------|------|-----------|------|
| Project Lead | ___________ | ___________ | _____ |
| Chief Pilot | ___________ | ___________ | _____ |
| Safety Officer | ___________ | ___________ | _____ |
