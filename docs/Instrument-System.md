# Instrument System
## Aircraft Instrumentation - Subsystem 2

**Document Version:** 1.0
**Date:** 2025-01-18
**Project:** N2-Arduino Aircraft Instrumentation
**System:** Instrument System (CAN ID Range: 0x200-0x2FF)

---

## Executive Summary

The Instrument System is responsible for flight instrumentation including altitude, attitude, heading, GPS navigation, fuel level, and battery monitoring. This system provides the pilot with critical flight data at 5Hz update rate with sensor fusion for stable attitude display.

**Platform:** Arduino Mega 2560 or Teensy 4.0
**Primary Function:** Flight instruments and navigation
**Update Rate:** 5Hz (200ms) for flight data, 1Hz for GPS
**CAN ID Range:** 0x200-0x2FF (256 message IDs)

**Key Benefits:**
- Multi-sensor fusion for accurate attitude
- GPS navigation and position tracking
- Altitude with QNH calibration support
- Battery and fuel monitoring
- Modular sensor architecture

---

## System Overview

### Hardware Platform

**Board:** Arduino Mega 2560 or Teensy 4.0
**Microcontroller:** ATmega2560 (16MHz, 256KB Flash, 8KB RAM) or ARM Cortex-M7 (600MHz)

### Key Components

- **Barometric Sensor:** BMP390 (I2C, 0x77)
  - Pressure: 300-1100 hPa range
  - Altitude: ±1m resolution
  - Internal temperature sensor

- **Inertial Measurement Unit:** BMI088 6-axis (I2C, 0x18/0x19)
  - Accelerometer: ±16g range
  - Gyroscope: ±2000°/s range
  - For pitch and roll calculation

- **Magnetometer:** BNO055 9-DOF (I2C, 0x28)
  - 3-axis magnetometer for heading
  - Built-in sensor fusion
  - 360° with 1° resolution

- **GPS Module:** NEO-6M (UART, 9600 baud)
  - Position: Latitude/Longitude
  - Altitude: GPS altitude (backup for barometer)
  - Speed and track over ground
  - Time synchronization

- **Fuel Level Sensor:** Resistive float (Analog, A0)
  - 0-100% range
  - 0-5V input

- **Battery Monitor:** Voltage divider (Analog, A1)
  - 0-20V range (scaled to 0-5V)
  - Current estimation from rate of change

- **Ambient Temperature:** DS18B20 (1-Wire, D22)
  - -55 to +125°C range
  - Digital sensor

- **CAN Interface:** MCP2515 + TJA1050 transceiver

### Sensor Pin Assignments

**Digital Pins:**
- D22: DS18B20 ambient temperature (1-Wire)
- D2-D3: Available for interrupts
- SPI: MCP2515 CAN controller

**Analog Pins:**
- A0: Fuel level sensor (0-5V)
- A1: Battery voltage (divided 0-5V)
- A2-A7: Available for expansion

**I2C Bus (Multiple devices):**
- BMP390 (0x77): Pressure/altitude
- BMI088 Accel (0x18): Accelerometer
- BMI088 Gyro (0x19): Gyroscope
- BNO055 (0x28): Magnetometer/heading

**UART:**
- Serial1: NEO-6M GPS (9600 baud)
- Serial0: USB debug (115200 baud)

---

## Processing Responsibilities

### Primary Functions

**1. Altitude and Pressure (5Hz)**
- Read BMP390 barometric sensor
- Calculate altitude using standard atmosphere model
- Apply QNH calibration (sea-level pressure correction)
- Vertical speed calculation (rate of altitude change)
- Transmit via CAN every 200ms

**2. Attitude Determination (5Hz)**
- Read BMI088 accelerometer and gyroscope
- Apply sensor fusion (Madgwick or complementary filter)
- Calculate pitch and roll angles
- Compensate for acceleration effects
- Transmit via CAN every 200ms

**3. Heading Calculation (5Hz)**
- Read BNO055 magnetometer
- Apply hard/soft iron calibration
- Tilt compensation using IMU data
- True heading calculation
- Transmit via CAN every 200ms

**4. GPS Processing (1Hz)**
- Parse NMEA sentences (RMC, GGA)
- Extract position (lat/lon)
- Extract ground speed and track
- Extract GPS altitude (backup)
- Extract UTC time for synchronization
- Transmit via CAN every 1 second

**5. Fuel and Battery Monitoring (2Hz)**
- Read fuel level sensor (analog)
- Apply calibration curve
- Calculate fuel flow rate (derivative)
- Read battery voltage
- Estimate current from voltage change rate
- Transmit via CAN every 500ms

**6. Temperature Monitoring (2Hz)**
- Read DS18B20 ambient temperature
- Read BMP390 internal temperature
- Transmit via CAN every 500ms

**7. Sensor Fusion and Validation**
- Combine IMU + magnetometer for stable heading
- Cross-check GPS altitude vs barometric altitude
- Detect sensor failures (timeout, out-of-range)
- Flag invalid data in status byte

---

## CAN Bus Protocol

### Transmitted Messages

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

### Received Messages

| Message ID | Name | Purpose |
|------------|------|---------|
| 0x400 | System Config | Receive QNH setting, units, calibration values |
| 0x401 | Time Sync | Synchronize internal clock (GPS provides time but sync needed) |

### Data Encoding

**GPS Coordinates (32-bit signed):**
- Format: degrees × 1,000,000
- Example: 52.5235°N → 52523500 → 0x03215F9C
- Byte order: Little-endian

**Angles (16-bit signed):**
- Format: degrees × 10
- Range: -180.0° to +180.0° (pitch/roll), 0-360° (heading)
- Example: +5.5° pitch → 55 → 0x0037

**Sensor Status Flags (Byte 0 of 0x206):**
```
Bit 7: Critical alert active
Bit 6: Warning alert active
Bit 5: Sensor fault detected
Bit 4: GPS fix valid (1 = fix, 0 = no fix)
Bit 3: IMU calibrated
Bit 2: Magnetometer calibrated
Bit 1: Barometer active
Bit 0: System armed/active
```

---

## Software Architecture

### Reusable Code from Current System

**Event-Driven Architecture (100% reusable)**
- EventManager library works on Arduino Mega/Teensy
- Same `handleEvents()` → `raiseEvents()` pattern
- May need larger queue size for multiple sensors

**Barometer Integration (90% reusable)**
- Current BMP085 code structure applies to BMP390
- Both are I2C, similar API
- QNH calibration logic transfers directly
- Altitude calculation identical (standard atmosphere)
- **Action:** Update library to BMP390, keep API same

**Memory Management Patterns (100% reusable)**
- `F()` macro for PROGMEM strings
- sprintf() to char buffers
- Fixed-size arrays for sensor data
- **Action:** Apply same patterns to instrument code

**Clock/RTC Handling (partial reuse)**
- Time synchronization concepts apply
- GPS provides time instead of RTC
- **Action:** Adapt for GPS NMEA time parsing

### New Development Required

**CAN Bus Integration**
- mcp_can library (same as Engine Monitor)
- Message encoding for all sensor types
- Multi-rate transmission scheduler (1Hz, 2Hz, 5Hz)
- Configuration reception handler

**GPS NMEA Parser**
- Parse RMC sentence: position, speed, track, time
- Parse GGA sentence: altitude, fix quality, satellites
- Checksum validation
- Buffer management for UART input
- Library option: TinyGPS++ (proven, well-tested)

**IMU Sensor Fusion**
- Madgwick filter or complementary filter
- Combine accelerometer + gyroscope
- Pitch and roll calculation
- Acceleration compensation (distinguish tilt from acceleration)
- Library option: Adafruit AHRS

**Magnetometer Calibration**
- Hard iron calibration (offset correction)
- Soft iron calibration (scaling/rotation)
- Tilt compensation (use IMU angles)
- Heading calculation from 3-axis mag data

**Altitude Calculation**
- Standard atmosphere model: h = (1 - (P/P₀)^0.190284) × 44330.77
- QNH correction: adjust P₀ based on QNH setting
- Vertical speed: derivative of altitude

**Fuel Flow Calculation**
- Track fuel level over time
- Calculate flow rate (gallons per hour)
- Filter noise (moving average)
- Detect refueling events (sudden increase)

**Battery Current Estimation**
- Measure voltage change rate (dV/dt)
- Estimate current: I ≈ C × (dV/dt)
- Requires battery capacity constant
- Low accuracy but useful trend indicator

### Memory Budget

**Flash Memory (Arduino Mega):**
- CAN library: ~6KB
- GPS library (TinyGPS++): ~8KB
- IMU fusion library: ~12KB
- Sensor drivers (BMP390, BMI088, BNO055): ~15KB
- EventManager: ~2KB
- Core application: ~17KB
- **Total: ~60KB of 256KB (23% utilization)**
- **Abundant space for future features**

**RAM (Arduino Mega):**
- CAN buffers: 256 bytes
- GPS parser buffer: 512 bytes
- IMU fusion state: 1KB
- Sensor data array: 512 bytes
- Event queues: ~64 bytes
- Stack/globals: ~1KB
- **Total: 3.3KB of 8KB (41% utilization)**
- **Ample headroom**

**EEPROM:**
- QNH setting (2 bytes): 0x00-0x01
- Fuel calibration (16 bytes): 0x02-0x11
- Magnetometer calibration (32 bytes): 0x12-0x31
- IMU calibration (32 bytes): 0x32-0x51
- **Total: 82 bytes**

---

## Power Budget

### Power Consumption

**Active Operation:**
- Arduino Mega: 50mA @ 5V = 250mW
- MCP2515 CAN: 5mA @ 5V = 25mW
- BMP390: 3mA @ 3.3V = 10mW
- BMI088: 5mA @ 3.3V = 17mW
- BNO055: 12mA @ 3.3V = 40mW
- GPS (tracking): 25mA @ 3.3V = 83mW
- DS18B20: 1mA @ 5V = 5mW
- Analog sensors: 10mA @ 5V = 50mW
- **Total Active: 111mA @ 5V = 480mW**

**GPS Acquisition (cold start):**
- GPS: 40mA @ 3.3V = 132mW
- **Total: 126mA @ 5V = 530mW (temporary)**

**Low Power Mode (future):**
- Reduce GPS to 7mA low-power mode
- IMU standby: 0.5mA
- **Total: ~35mA @ 5V = 175mW**

### From 12V Aircraft Bus
- With 95% efficient regulator: ~48mA @ 12V
- Acceptable for aircraft electrical system

---

## Development Plan

### Phase 1: Hardware Setup (Weeks 1-2)

**Week 1: Component Assembly**
- Procure components (Arduino Mega, sensors, MCP2515)
- Identify I2C address conflicts (use I2C scanner)
- Assemble on breadboard
- Verify each sensor initializes:
  - BMP390 responds on I2C
  - BMI088 accel/gyro detected
  - BNO055 enters fusion mode
  - GPS outputs NMEA sentences
  - Fuel and battery sensors read valid voltage

**Week 2: Sensor Testing**
- Test BMP390 altitude (compare to known elevation)
- Test IMU angles (compare to level surface)
- Test magnetometer heading (compare to compass)
- Test GPS fix (cold start outdoors <60s)
- Calibrate fuel level sensor (empty/full)
- Calibrate battery voltage divider

**Deliverables:**
- Functioning instrument hardware
- All sensors initialized and reading
- Basic calibration complete

### Phase 2: Software Development (Weeks 3-6)

**Week 3: Core Sensor Reading**
- Port EventManager library
- Implement BMP390 altitude calculation
- Implement GPS NMEA parser (TinyGPS++ library)
- Create sensor data storage structure
- Implement 5Hz sensor reading loop

**Week 4: IMU Sensor Fusion**
- Integrate Adafruit AHRS library (or Madgwick filter)
- Combine BMI088 accelerometer + gyroscope
- Calculate pitch and roll
- Test accuracy against level surface (±2°)
- Integrate BNO055 for heading

**Week 5: CAN Integration**
- Integrate mcp_can library
- Implement message encoding for all data types
- Create multi-rate scheduler (1Hz, 2Hz, 5Hz)
- Implement heartbeat transmission
- Test with CAN analyzer

**Week 6: Advanced Features**
- QNH calibration support
- Fuel flow calculation
- Battery current estimation
- Sensor failure detection
- Configuration message reception

**Deliverables:**
- Instrument System transmitting all CAN messages
- Sensor fusion working (stable attitude)
- GPS fix and position valid
- Calibration system functional

### Testing Milestones

**Altitude Accuracy:**
- Within ±10 ft of known elevation
- QNH adjustment works (compare to METAR)
- Vertical speed reasonable (±100 ft/min)

**Attitude Accuracy:**
- Pitch within ±2° of level surface
- Roll within ±2° of level surface
- Stable under vibration
- No drift over 1 minute

**Heading Accuracy:**
- Within ±5° of magnetic compass
- Tilt compensation works (accurate when pitched/rolled)
- Calibration procedure reduces error

**GPS Performance:**
- Cold start fix <60 seconds
- Warm start fix <30 seconds
- Position accurate (within 10m of known location)
- Speed and track reasonable

**CAN Communication:**
- All messages transmitted at correct rate
- Verified with CAN analyzer
- Configuration reception working

---

## Sensor Fusion Algorithms

### Attitude Calculation (Pitch/Roll)

**Complementary Filter Approach:**
```
// Accelerometer provides gravity vector (slow, no drift)
pitch_accel = atan2(ay, sqrt(ax² + az²))
roll_accel = atan2(-ax, az)

// Gyroscope provides rate of change (fast, drifts)
pitch_rate = gyro_x * dt
roll_rate = gyro_y * dt

// Complementary filter (98% gyro, 2% accel)
pitch = 0.98 * (pitch + pitch_rate) + 0.02 * pitch_accel
roll = 0.98 * (roll + roll_rate) + 0.02 * roll_accel
```

**Madgwick Filter (recommended):**
- More sophisticated algorithm
- Better performance under acceleration
- Adafruit AHRS library implementation available
- Adjustable beta parameter (filter aggressiveness)

### Heading Calculation

**Magnetometer Tilt Compensation:**
```
// Compensate for pitch and roll
mag_x_comp = mag_x * cos(pitch) + mag_z * sin(pitch)
mag_y_comp = mag_x * sin(roll) * sin(pitch) + mag_y * cos(roll) - mag_z * sin(roll) * cos(pitch)

// Calculate heading
heading = atan2(mag_y_comp, mag_x_comp) * 180 / PI

// Apply declination correction
heading += magnetic_declination
if (heading < 0) heading += 360
if (heading >= 360) heading -= 360
```

**Calibration:**
- Hard iron: Offset all axes (remove constant bias)
- Soft iron: Scale and rotate (correct for distortion)
- Procedure: Rotate sensor through all orientations, record min/max

---

## Calibration Procedures

### QNH Setting (Altimeter)

**Method 1: Known Elevation**
1. Obtain field elevation (airport chart)
2. Read current pressure altitude
3. Adjust QNH until altitude matches field elevation
4. Transmit QNH to system via Pilot Info

**Method 2: METAR/ATIS**
1. Obtain current altimeter setting from METAR/ATIS
2. Enter QNH value via Pilot Info touchscreen
3. System applies correction automatically

**Standard Atmosphere:**
- QNH = 1013.25 hPa (29.92 inHg) at sea level
- Pressure decreases ~1 hPa per 30 ft altitude gain
- Temperature standard: 15°C at sea level, -2°C per 1000 ft

### Fuel Level Sensor

**Calibration Curve:**
- Measure voltage at empty tank: V_empty
- Measure voltage at full tank: V_full
- Linear interpolation: Fuel% = (V - V_empty) / (V_full - V_empty) × 100

**Non-linear Correction:**
- Some tanks are non-linear (tapered shape)
- Create lookup table: voltage → gallons
- Store in EEPROM (16-point table)

### Battery Voltage Divider

**Voltage Scaling:**
- Resistor divider: R1=15kΩ, R2=5kΩ (divide by 4)
- Input: 0-20V → Output: 0-5V
- ADC reading: V_battery = ADC × (5.0 / 1023) × 4.0
- Verify with multimeter, adjust scaling factor if needed

### Magnetometer Calibration

**Hard Iron Calibration:**
1. Rotate sensor through full sphere of orientations
2. Record min/max for each axis
3. Offset = (max + min) / 2
4. Apply: mag_calibrated = mag_raw - offset

**Soft Iron Calibration:**
1. Advanced: Fit ellipsoid to data
2. Calculate transformation matrix
3. Apply: mag_calibrated = matrix × (mag_raw - offset)
4. Use calibration library (e.g., Freescale AN4246)

---

## Integration with Other Subsystems

### Data Flow

**To Data Logger:**
- All flight data logged at 5Hz
- GPS track logged at 1Hz
- Sensor status logged

**To Pilot Information:**
- Altitude displayed on flight instruments screen
- Attitude displayed (artificial horizon)
- Heading displayed (compass rose)
- GPS position on navigation screen
- Fuel and battery on status screen

**From Pilot Information:**
- QNH setting (altimeter correction)
- Units configuration (metric/imperial, ft/m, knots/km/h)
- Sensor calibration values
- Magnetic declination

### Fault Tolerance

**Sensor Redundancy:**
- GPS altitude backs up barometric altitude
- Internal temperature backs up ambient sensor
- Cross-check data between sensors

**Graceful Degradation:**
- Loss of GPS: Continue with barometer, no position/speed
- Loss of magnetometer: Heading shows "---"
- Loss of IMU: Attitude shows "---"
- Loss of barometer: Use GPS altitude if available

**Watchdog and Recovery:**
- Software watchdog monitors main loop
- Reset if hung (restore within 2 seconds)
- Sensor re-initialization on fault

---

## Testing and Validation

### Bench Testing

**Altitude Test:**
- Set known QNH
- Compare to surveyed elevation
- Verify within ±10 ft
- Test QNH adjustment (±10 hPa change)

**Attitude Test:**
- Place on level surface: pitch=0°, roll=0°
- Tilt 10°: verify readings within ±2°
- Rotate continuously: check for drift
- Shake (simulate vibration): readings stable

**Heading Test:**
- Compare to magnetic compass
- Rotate 360°: verify continuous tracking
- Tilt while rotating: tilt compensation works
- Check for local magnetic interference

**GPS Test:**
- Cold start outdoors: fix <60s
- Compare position to known location (within 10m)
- Drive/walk: speed and track reasonable
- Stationary: position stable (±5m)

### Integration Testing

**With Engine Monitor:**
- Verify both systems on same CAN bus
- No conflicts or bus overload
- Timing validated

**With Data Logger:**
- All data logged correctly
- Timestamps accurate
- GPS time used for synchronization

**With Pilot Information:**
- QNH changes reflected immediately
- Attitude display smooth and responsive
- GPS track displayed on navigation screen

### Field Testing

**Ground Testing:**
- Engine vibration test (sensors stable?)
- Electrical noise test (readings clean?)
- Temperature range test (-20°C to +50°C)

**Taxi Testing:**
- GPS tracks movement accurately
- Attitude responds to turns
- No sensor failures from vibration

**Flight Testing:**
- Altitude tracks climb/descent
- Attitude accurate in flight (compare to visual)
- GPS position accurate (compare to landmarks)
- Heading tracks turns
- Fuel and battery monitoring functional

---

## Bill of Materials

| Component | Quantity | Est. Cost | Source | Notes |
|-----------|----------|-----------|--------|-------|
| Arduino Mega 2560 | 1 | $40 | Amazon, Adafruit | ATmega2560 |
| MCP2515 CAN Module | 1 | $8 | Amazon | With TJA1050 |
| BMP390 Sensor | 1 | $15 | Adafruit | Barometric pressure |
| BMI088 IMU | 1 | $25 | Adafruit | 6-axis accel+gyro |
| BNO055 9-DOF | 1 | $35 | Adafruit | Magnetometer+fusion |
| NEO-6M GPS Module | 1 | $18 | Amazon | With antenna |
| DS18B20 Temp Sensor | 1 | $3 | Adafruit | 1-Wire digital |
| Fuel Level Sensor | 1 | $40 | Aircraft Spruce | Float type, 0-100% |
| Voltage Divider | 1 | $2 | Local | Resistors for battery |
| Prototype Shield | 1 | $12 | Adafruit | For Mega |
| Power Supply (5V) | 1 | $12 | Amazon | Buck converter |
| Enclosure | 1 | $20 | Hammond | 6"×4"×2" |
| Connectors & Wire | - | $30 | DigiKey | Various |
| **Total** | | **$260** | | Approximate |

---

## Document Control

**Version:** 1.0
**Date:** 2025-01-18
**Related Documents:**
- Engine-Monitor-System.md
- Data-Logger-System.md
- Pilot-Information-System.md
- Multi-System-Architecture-Plan.md (complete system)

**Change History:**

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2025-01-18 | Initial document created from multi-system plan |
