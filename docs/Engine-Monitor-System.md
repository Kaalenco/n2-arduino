# Engine Monitor System
## Aircraft Instrumentation - Subsystem 1

**Document Version:** 1.0
**Date:** 2025-01-18
**Project:** N2-Arduino Aircraft Instrumentation
**System:** Engine Monitor (CAN ID Range: 0x100-0x1FF)

---

## Executive Summary

The Engine Monitor System is a safety-critical subsystem responsible for real-time monitoring of all engine parameters. This system reads temperature, pressure, and RPM data at high frequency (10Hz) and provides immediate fault detection and alerting capabilities.

**Platform:** Arduino Nano 33 IoT or Arduino Uno + MCP2515 CAN controller
**Primary Function:** Engine parameter monitoring and protection
**Update Rate:** 10Hz (100ms) for real-time response
**CAN ID Range:** 0x100-0x1FF (256 message IDs)

**Key Benefits:**
- Fast response time for engine protection (<100ms)
- Dedicated hardware for safety-critical monitoring
- Fault isolation from display and logging systems
- Low power consumption (~380mW active)
- Real-time engine hours tracking

---

## System Overview

### Hardware Platform

**Board:** Arduino Nano 33 IoT or Arduino Uno
**Microcontroller:** ATmega328P (32KB Flash, 2KB RAM)
**CAN Controller:** MCP2515 (SPI interface) + TJA1050 transceiver

### Key Components

- **Temperature Sensors (9×):** MAX6675 K-type thermocouple interfaces
  - 4× EGT (Exhaust Gas Temperature) sensors: 0-900°C range
  - 4× CHT (Cylinder Head Temperature) sensors: 0-300°C range
  - 1× Oil Temperature sensor: 0-150°C range
- **Pressure Sensors (2×):** Analog inputs
  - Oil Pressure: 0-100 PSI (voltage divider on A4)
  - Manifold Pressure: 10-30 inHg (for turbo monitoring on A5)
- **Tachometer Input:** Frequency counter on D2 (INT0), 0-6000 RPM
- **Real-Time Clock:** DS3231 (I2C) for engine hours tracking
- **Status Indicator:** LED for engine fault indication
- **CAN Interface:** MCP2515 for network communication

### Sensor Pin Assignments

**Digital Pins:**
- D2: Tachometer input (INT0 interrupt)
- D9: Oil Temperature MAX6675 (CS)
- D10-D13: EGT Sensors 1-4 MAX6675 (CS)

**Analog Pins:**
- A0-A3: CHT Sensors 1-4 MAX6675 (CS - used as digital)
- A4: Oil Pressure sensor (analog input)
- A5: Manifold Pressure sensor (analog input)

**I2C (Shared):**
- DS3231 RTC for engine hours tracking
- MCP2515 uses SPI (MOSI, MISO, SCK, CS)

---

## Processing Responsibilities

### Primary Functions

1. **High-Frequency Sensor Reading (10Hz)**
   - Read all 9 temperature sensors every 100ms
   - Measure RPM from tachometer frequency input
   - Read oil pressure and manifold pressure
   - Minimal latency for protective monitoring

2. **Engine Hours Tracking**
   - Track cumulative running time when RPM > 500
   - Persist to EEPROM every 0.1 hour (6 minutes)
   - Survives power cycles
   - Displayed in hours with decimal precision

3. **Limit Checking and Alerts**
   - **EGT Limits:** Warn >750°C, Critical >800°C
   - **CHT Limits:** Warn >200°C, Critical >230°C
   - **Oil Temperature:** Warn >120°C, Critical >130°C
   - **Oil Pressure:** Warn <20 PSI at cruise RPM
   - Flash fault LED if critical limit exceeded
   - Transmit alert status via CAN

4. **CAN Bus Communication**
   - Transmit engine data every 100ms (10Hz)
   - Send status updates at 1Hz
   - Receive configuration from Pilot Info system
   - Heartbeat transmission for fault detection

### Alert Thresholds

| Parameter | Warning | Critical | Action |
|-----------|---------|----------|--------|
| EGT (any cylinder) | >750°C | >800°C | Flash LED, CAN alert |
| CHT (any cylinder) | >200°C | >230°C | Flash LED, CAN alert |
| Oil Temperature | >120°C | >130°C | Flash LED, CAN alert |
| Oil Pressure | <30 PSI | <20 PSI | Flash LED, CAN alert |
| RPM | >2700 | >2900 | Flash LED, CAN alert |

---

## CAN Bus Protocol

### Transmitted Messages

| Message ID | Name | Frequency | Data Layout |
|------------|------|-----------|-------------|
| 0x100 | EGT Temperatures | 10Hz | Byte 0-1: EGT1 (°C), 2-3: EGT2, 4-5: EGT3, 6-7: EGT4 |
| 0x101 | CHT Temperatures | 10Hz | Byte 0-1: CHT1 (°C), 2-3: CHT2, 4-5: CHT3, 6-7: CHT4 |
| 0x102 | Oil & Manifold | 10Hz | Byte 0-1: Oil Temp (°C), 2-3: Oil Pressure (PSI×10), 4-5: MAP (inHg×10) |
| 0x103 | Tach & Hours | 1Hz | Byte 0-1: RPM, 2-5: Engine Hours (seconds) |
| 0x104 | Engine Status | 1Hz | Byte 0: Status flags, 1: Max EGT, 2: Max CHT, 3-7: Reserved |
| 0x10F | Heartbeat | 1Hz | Byte 0: Node ID (0x01), 1: Uptime (minutes), 2-7: Reserved |

### Received Messages

| Message ID | Name | Purpose |
|------------|------|---------|
| 0x400 | System Config | Receive alert limits, units configuration |
| 0x401 | Time Sync | Synchronize RTC with Pilot Info system |

### Data Encoding

**Temperature (16-bit signed):**
- Format: °C × 10 (e.g., 195.5°C → 1955)
- Byte order: Little-endian (LSB first)
- Example: 195.5°C → 0x07A3 → Byte[0]=0xA3, Byte[1]=0x07

**Pressure (16-bit unsigned):**
- Format: PSI × 10 or inHg × 10
- Byte order: Little-endian
- Example: 55.2 PSI → 552 → 0x0228

**Status Flags (Byte 0 of 0x104):**
```
Bit 7: Critical alert active
Bit 6: Warning alert active
Bit 5: Sensor fault detected
Bit 4: Reserved
Bit 3: Reserved
Bit 2: Reserved
Bit 1: Reserved
Bit 0: Engine running (RPM > 500)
```

---

## Software Architecture

### Reusable Code from Current System

**Event-Driven Architecture (100% reusable)**
- EventManager library works unchanged on Arduino Nano
- `handleEvents()` → `raiseEvents()` pattern proven effective
- Fixed-size queues (8 events) and listeners (8) appropriate

**MAX6675 Sensor Reading (95% reusable)**
- Current code reads 4× MAX6675 sensors via SPI
- Engine Monitor extends to 9× MAX6675 sensors
- Same reading logic: `tempEGT.readCelsius()` → store in array
- **Action:** Extend to support 9 CS pins, same reading pattern

**Memory Management Patterns (100% reusable)**
- `F()` macro for PROGMEM strings
- sprintf() to char buffers (avoid String objects)
- Fixed-size arrays for sensor data
- **Action:** Apply same patterns in all engine monitor code

**Clock/RTC Handling (100% reusable)**
- DS3231 RTC code works unchanged
- Engine hours tracking uses same time-keeping approach
- **Action:** Copy clock library for hours tracking

### New Development Required

**CAN Bus Integration**
- mcp_can library integration (6KB Flash)
- Message encoding functions (temperature, pressure, RPM)
- Message transmission scheduler (10Hz for temps, 1Hz for status)
- Configuration message reception handler

**Tachometer Frequency Measurement**
- Timer1 or interrupt-based frequency counter
- RPM calculation from pulse frequency
- Debouncing and noise filtering
- Range checking (0-6000 RPM valid)

**Engine Hours Tracking**
- Increment when RPM > 500 (engine running threshold)
- EEPROM persistence every 0.1 hour (reduce wear)
- Read on startup, display on request
- Transmit via CAN every 1 second

**Limit Checking and Alert Generation**
- Compare all sensor values against thresholds
- Generate warning/critical flags
- Flash LED on critical alert
- Encode status into CAN message 0x104

**Watchdog Timer Implementation**
- Hardware watchdog (15ms - 8s timeout)
- Reset if main loop hangs
- Critical for safety system

### Memory Budget

**Flash Memory:**
- CAN library: ~6KB
- EventManager library: ~2KB
- MAX6675 drivers (9×): ~3KB
- Core application logic: ~7KB
- **Total: ~18KB of 32KB (56% utilization)**
- **Available: ~14KB for future features**

**RAM:**
- CAN buffers: 256 bytes
- Sensor data array: 128 bytes (9 temps + 2 pressures + RPM + hours)
- Event queues: ~64 bytes (1 EventManager × 8 events)
- Stack/globals: ~800 bytes
- **Total: 1,248 bytes of 2,048 bytes (61% utilization)**
- **Available: ~800 bytes**

**EEPROM:**
- Engine hours (4 bytes): 0x00-0x03
- Alert limit configuration (32 bytes): 0x04-0x23
- Calibration data (32 bytes): 0x24-0x43
- **Total: 68 bytes of 1024 bytes**

---

## Power Budget

### Power Consumption

**Active Operation:**
- Arduino Nano: 20mA @ 5V = 100mW
- MCP2515 CAN: 5mA @ 5V = 25mW
- MAX6675 ×9 (reading): 50mA @ 5V = 250mW (1.5mA standby each)
- DS3231 RTC: 1mA @ 5V = 5mW
- **Total Active: 76mA @ 5V = 380mW**

**Idle (between sensor reads):**
- Arduino + CAN: 25mA @ 5V
- MAX6675 standby: ~2mA @ 5V
- **Total Idle: 27mA @ 5V = 135mW**

**Peak (all sensors reading simultaneously):**
- **126mA @ 5V = 630mW**

### From 12V Aircraft Bus
- With 95% efficient regulator: ~40mA @ 12V
- Acceptable for aircraft electrical system

---

## Development Plan

### Phase 1: Hardware Setup (Weeks 1-2)

**Tasks:**
- Procure components (Arduino Nano, MCP2515, 9× MAX6675, DS3231)
- Assemble on breadboard/prototype shield
- Verify all sensor connections:
  - 9× SPI CS lines for MAX6675
  - 2× analog inputs (oil pressure, manifold pressure)
  - 1× frequency input (tachometer)
  - I2C for RTC
  - SPI for CAN controller
- Test each sensor individually

**Deliverables:**
- Functioning hardware prototype
- All 9 temperature sensors reading correctly
- Analog sensors calibrated
- Tachometer input verified with function generator

**Testing:**
- Verify SPI communication with all MAX6675 sensors
- Check for noise/interference on long SPI bus
- Test analog sensors with known voltage sources
- Verify tachometer frequency measurement (0-6000 RPM range)

### Phase 2: Software Development (Weeks 3-4)

**Week 3: Core Functionality**
- Port EventManager library from current system
- Extend MAX6675 reading code to support 9 sensors
- Implement tachometer frequency measurement (Timer1)
- Create sensor data storage array
- Implement basic sensor reading loop (10Hz)

**Week 4: CAN Integration**
- Integrate mcp_can library
- Implement CAN message encoding functions
- Create message transmission scheduler
- Implement heartbeat transmission
- Add configuration message reception

**Week 5: Advanced Features**
- Implement engine hours tracking with EEPROM
- Add limit checking and alert generation
- Implement fault LED control
- Add watchdog timer
- Create status message generation

**Deliverables:**
- Engine Monitor transmitting all CAN messages at correct rates
- Engine hours tracking and persistence working
- Alert system functional
- Watchdog timer operational

### Testing Milestones

**Sensor Accuracy:**
- All 9 temperature sensors read within ±2°C of known value
- Oil pressure sensor reads within ±2 PSI
- Manifold pressure sensor reads within ±0.5 inHg
- Tachometer accurate within ±10 RPM

**CAN Communication:**
- All messages transmitted at required rate (±10% jitter)
- Message format verified with CAN analyzer
- Heartbeat regular at 1Hz
- Configuration reception working

**Engine Hours:**
- Accumulates time when RPM > 500
- Persists across power cycles
- Accurate within ±1 second per hour
- EEPROM write count managed (every 0.1 hour)

**Alert System:**
- Triggers within 100ms of threshold crossing
- LED flashes on critical alert
- Status transmitted on CAN bus
- Clears when parameter returns to safe range

### Risk Mitigation

**MAX6675 SPI Noise Issues:**
- **Risk:** Long wires to distributed thermocouples cause noise
- **Mitigation:**
  - Keep wires short (<6 inches if possible)
  - Add 100nF ceramic caps near each MAX6675
  - Use slower SPI speed (<1MHz)
  - Add delays between sensor reads

**Tachometer False Triggering:**
- **Risk:** Ignition noise causes false RPM readings
- **Mitigation:**
  - Add RC low-pass filter on input
  - Software debouncing
  - Range checking (reject >6000 RPM)
  - Compare to previous value (reject >500 RPM change)

**EEPROM Wear:**
- **Risk:** Frequent writes wear out EEPROM
- **Mitigation:**
  - Write only every 0.1 hour (100K writes = 10,000 hours)
  - Rotate write locations (wear leveling)
  - Use EEPROM update function (write only if changed)

---

## Integration with Other Subsystems

### Data Flow

**To Data Logger (0x300 range):**
- All engine data logged at 10Hz
- Status and alerts logged at 1Hz
- Engine hours changes logged

**To Pilot Information (0x400 range):**
- All engine data displayed in real-time
- Alerts trigger visual/audio warnings
- Engine hours displayed on engine screen

**From Pilot Information:**
- Alert limit configuration (customizable thresholds)
- Units configuration (metric/imperial)
- Time synchronization for RTC

### Fault Tolerance

**Independent Operation:**
- Continues monitoring even if CAN bus fails
- Can operate with local LED indication only
- Engine hours tracking continues regardless of network

**Watchdog Recovery:**
- Hardware watchdog resets system if hung
- Restores to monitoring state within 1 second
- Engine hours preserved in EEPROM

**Sensor Failure Handling:**
- Individual sensor failures detected
- Fault flag transmitted on CAN
- System continues with remaining sensors
- Missing sensors don't prevent operation

**Heartbeat Monitoring:**
- Transmits heartbeat every 1 second
- Other systems detect failure within 3 seconds
- Pilot Info displays "ENGINE MONITOR OFFLINE" warning

---

## Testing and Validation

### Unit Testing

**Sensor Reading:**
- Test each MAX6675 with known temperature source
- Verify analog sensors with precision voltage source
- Validate tachometer with function generator

**CAN Messaging:**
- Verify message format with CAN analyzer
- Check timing with oscilloscope
- Validate data encoding/decoding

**Engine Hours:**
- Test accumulation accuracy over time
- Verify EEPROM persistence
- Check wear leveling strategy

### Integration Testing

**With Data Logger:**
- Verify all messages logged correctly
- Check timestamp accuracy
- Validate data format in CSV

**With Pilot Information:**
- Verify real-time display updates
- Test alert triggering and display
- Validate configuration sync

### System Testing

**Full Flight Simulation:**
- Startup: RPM ramps from 0 to 2000, temps rise
- Cruise: Steady-state parameters
- Shutdown: RPM drops, temps cool
- Verify engine hours increment correctly

**Fault Injection:**
- Disconnect sensors (verify fault detection)
- Exceed thresholds (verify alerts)
- Kill CAN bus (verify independent operation)
- Power cycle (verify recovery)

### Field Testing

**Ground Runs:**
- Monitor actual engine parameters
- Compare to factory gauges
- Validate temperature readings
- Check RPM accuracy against tachometer

**Flight Testing:**
- Monitor during takeoff (high power)
- Cruise monitoring (steady-state)
- Validate oil pressure under load
- Check for vibration-induced noise

---

## Maintenance and Troubleshooting

### Pre-Flight Checks

- Verify LED blinks on power-up (self-test)
- Check for any fault codes on Pilot Info display
- Verify engine hours displayed correctly
- Confirm all temperature sensors active

### Diagnostic Procedures

**LED Flash Codes:**
- Solid: Normal operation, no alerts
- Slow flash (1Hz): Warning alert active
- Fast flash (5Hz): Critical alert active
- Off: Power failure or system fault

**Serial Debug Output:**
- Connect USB serial at 115200 baud
- View sensor readings in real-time
- Check CAN message transmission status
- Review error logs

**Common Issues:**

| Symptom | Cause | Solution |
|---------|-------|----------|
| Temperature reads -1 or 999 | MAX6675 connection issue | Check SPI wiring, CS pin |
| RPM reads 0 | No tachometer signal | Check tach wire connection |
| Engine hours reset | EEPROM corruption | Manually set hours via config |
| No CAN messages | MCP2515 issue | Check CAN wiring, termination |
| False alerts | Wrong thresholds | Reconfigure via Pilot Info |

### Calibration Procedures

**Temperature Sensors:**
- No calibration needed (MAX6675 factory calibrated)
- Verify with boiling water (100°C at sea level)

**Oil Pressure Sensor:**
- Record voltage at known pressure (mechanical gauge)
- Update scaling factor in code: `pressure_psi = (voltage * scale) + offset`

**Tachometer:**
- Compare to factory tachometer at various RPM
- Adjust scaling factor if needed: `rpm = (frequency * 60) / pulses_per_rev`

### Software Updates

**Upload Procedure:**
1. Connect USB cable to Arduino
2. Open PlatformIO project
3. Build firmware: `pio run`
4. Upload: `pio run --target upload`
5. Verify via serial monitor
6. Test on bench before flight

**Backup Configuration:**
- Note current engine hours before update
- Save alert thresholds
- Restore via Pilot Info system after update

---

## Bill of Materials

| Component | Quantity | Est. Cost | Source | Notes |
|-----------|----------|-----------|--------|-------|
| Arduino Nano | 1 | $15 | Adafruit, SparkFun | ATmega328P version |
| MCP2515 CAN Module | 1 | $8 | Amazon, eBay | Includes TJA1050 |
| MAX6675 Thermocouple | 9 | $45 | Amazon ($5 each) | K-type, SPI |
| K-Type Thermocouple | 9 | $90 | Aircraft Spruce | 1/4" NPT, 0-900°C |
| Oil Pressure Sensor | 1 | $25 | Aircraft Spruce | 0-100 PSI, 0-5V |
| Manifold Pressure | 1 | $30 | Aircraft Spruce | Optional for turbo |
| DS3231 RTC Module | 1 | $5 | Amazon | Battery backup |
| Prototype Shield | 1 | $8 | Adafruit | For Arduino Nano |
| Status LED | 1 | $1 | Local electronics | Red, high brightness |
| Power Supply (5V) | 1 | $12 | Amazon | Buck converter, 12V→5V |
| Enclosure | 1 | $15 | Hammond | 4"×3"×2" plastic |
| Connectors & Wire | - | $30 | DigiKey | Terminal blocks, wire |
| **Total** | | **$284** | | Approximate |

---

## Document Control

**Version:** 1.0
**Date:** 2025-01-18
**Related Documents:**
- Instrument-System.md
- Data-Logger-System.md
- Pilot-Information-System.md
- Multi-System-Architecture-Plan.md (complete system)

**Change History:**

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2025-01-18 | Initial document created from multi-system plan |
