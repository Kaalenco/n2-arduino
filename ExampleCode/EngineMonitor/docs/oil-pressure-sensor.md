# Oil Pressure Sensor - VDO 360-081-030-015C

## Sensor Specifications

- **Pressure range**: 0 to 10 Bar
- **Thread**: 1/8 - 27 NPTF
- **Sender resistance range**: 10Ω (0 Bar) to 184Ω (10 Bar)
- **Sensor signal**: 1-pole common ground (G connector)
- **Warning contact**: 0.80 Bar ± 0.30 Bar (WK connector - not in use)
- **Operating voltage**: 6-24V DC

## Circuit Design

### Overview

The oil pressure sensor is a variable resistance sender. We measure pressure by passing a known current through the sensor and measuring the resulting voltage/current relationship using an INA226 current/power monitor.

### Component Selection

**Series Resistor (R1): 100Ω 1W**
- Chosen to maximize current range while staying within safe limits
- At 0 Bar (10Ω): I = 12V / 110Ω = 109 mA
- At 10 Bar (184Ω): I = 12V / 284Ω = 42 mA
- Current range: ~67 mA (good resolution)
- Max power dissipation: 1.2W (use 2W resistor for safety margin)

**INA226 Shunt Resistor: 0.1Ω**
- Voltage drop at max current: 109mA × 0.1Ω = 10.9mV
- Well within INA226 measurement range

### Protection Components

```
+12V ────┬──[F1]──┬──[D1]──┬──[R1]──┬──[INA226 IN+]
         │        │        │        │
        [C1]     [D2]     [C2]    [R_shunt]
         │        │        │        │
        GND      GND      GND   [INA226 IN-]
                                    │
                              [Sensor G pin]
                                    │
                                   GND
```

| Component | Value | Purpose |
|-----------|-------|---------|
| F1 | 250mA PTC resettable fuse | Overcurrent protection |
| D1 | 1N5819 Schottky | Reverse polarity protection |
| D2 | TVS diode 15V | Transient voltage suppression |
| C1 | 100µF 25V electrolytic | Bulk filtering |
| C2 | 100nF ceramic | High-frequency noise filtering |
| R1 | 100Ω 2W | Current limiting resistor |

### I2C Protection (Optional but recommended)

Add to SDA and SCL lines:
- 4.7kΩ pull-up resistors to 5V (if not on INA226 module)
- Series 100Ω resistors for ESD protection
- TVS diode array (e.g., PRTR5V0U2X) for ESD protection

## Wiring Diagram

```
Arduino             INA226              Sensor Circuit
--------           --------            ----------------
   5V  ────────────  VCC
  GND  ────────────  GND
  SDA  ────────────  SDA
  SCL  ────────────  SCL
                     VBS  ──────────── +12V (via protection)
                     IN+  ──────────── After R1
                     IN-  ──────────── To sensor G pin
                                            │
                                           GND
```

## Code Implementation

```cpp
#include <Wire.h>
#include <INA226_WE.h>

// ============================================================================
// Configuration
// ============================================================================

#define I2C_ADDRESS_OIL_PRESSURE 0x40  // INA226 I2C address

// Circuit constants
const float SERIES_RESISTOR = 100.0;    // Ohms
const float SHUNT_RESISTOR = 0.1;       // Ohms
const float SUPPLY_VOLTAGE = 12.0;      // Nominal supply voltage

// Sensor calibration (VDO 360-081-030-015C)
const float SENSOR_R_MIN = 10.0;        // Ohms at 0 Bar
const float SENSOR_R_MAX = 184.0;       // Ohms at 10 Bar
const float PRESSURE_MIN = 0.0;         // Bar
const float PRESSURE_MAX = 10.0;        // Bar

// Safety limits
const float CURRENT_MIN_VALID = 30.0;   // mA - below this indicates open circuit
const float CURRENT_MAX_VALID = 120.0;  // mA - above this indicates short circuit
const float PRESSURE_WARNING = 1.5;     // Bar - low pressure warning threshold
const float PRESSURE_CRITICAL = 0.8;    // Bar - critical low pressure

// Filtering
const int FILTER_SAMPLES = 8;           // Number of samples for averaging
const int SAMPLE_INTERVAL_MS = 50;      // Milliseconds between samples

// ============================================================================
// Global Variables
// ============================================================================

INA226_WE ina226_oil(I2C_ADDRESS_OIL_PRESSURE);

// Sensor state
bool oilPressureSensorActive = false;
float oilPressureBar = 0.0;
float oilPressurePsi = 0.0;
uint8_t oilPressureStatus = 0;  // 0=OK, 1=Warning, 2=Critical, 3=Error

// Filter buffer
float pressureBuffer[FILTER_SAMPLES];
int bufferIndex = 0;
bool bufferFilled = false;

// ============================================================================
// Initialization
// ============================================================================

bool initOilPressureSensor() {
    Serial.print(F("Initializing oil pressure sensor... "));

    // Initialize I2C with timeout
    Wire.begin();
    Wire.setWireTimeout(3000, true);  // 3ms timeout, reset on timeout

    // Check if INA226 responds
    Wire.beginTransmission(I2C_ADDRESS_OIL_PRESSURE);
    if (Wire.endTransmission() != 0) {
        Serial.println(F("FAILED - INA226 not found"));
        oilPressureSensorActive = false;
        return false;
    }

    // Initialize INA226
    if (!ina226_oil.init()) {
        Serial.println(F("FAILED - INA226 init error"));
        oilPressureSensorActive = false;
        return false;
    }

    // Configure INA226
    ina226_oil.setResistorRange(SHUNT_RESISTOR, 0.2);  // 0.1Ω shunt, 200mA max
    ina226_oil.setAverage(AVERAGE_16);                  // Hardware averaging
    ina226_oil.setConversionTime(CONV_TIME_1100);       // 1.1ms conversion
    ina226_oil.setMeasureMode(CONTINUOUS);              // Continuous measurement

    // Wait for first valid reading
    delay(50);

    // Verify we get valid readings
    float testCurrent = ina226_oil.getCurrent_mA();
    if (testCurrent < CURRENT_MIN_VALID || testCurrent > CURRENT_MAX_VALID) {
        Serial.print(F("WARNING - Unusual current: "));
        Serial.print(testCurrent);
        Serial.println(F(" mA"));
    }

    // Initialize filter buffer
    for (int i = 0; i < FILTER_SAMPLES; i++) {
        pressureBuffer[i] = 0.0;
    }
    bufferIndex = 0;
    bufferFilled = false;

    oilPressureSensorActive = true;
    Serial.println(F("OK"));

    return true;
}

// ============================================================================
// Reading Functions
// ============================================================================

/**
 * Read raw current from INA226 with error handling
 * Returns current in mA, or -1.0 on error
 */
float readOilPressureCurrentRaw() {
    if (!oilPressureSensorActive) {
        return -1.0;
    }

    // Read current with overflow check
    if (ina226_oil.overflow) {
        Serial.println(F("OilP: INA226 overflow"));
        return -1.0;
    }

    float current_mA = ina226_oil.getCurrent_mA();

    // Sanity check
    if (isnan(current_mA) || isinf(current_mA)) {
        Serial.println(F("OilP: Invalid current reading"));
        return -1.0;
    }

    return current_mA;
}

/**
 * Convert current reading to pressure in Bar
 */
float currentToPressure(float current_mA, float busVoltage_V) {
    if (current_mA <= 0) {
        return -1.0;
    }

    // Calculate sensor resistance from measured current
    // V = I * (R_series + R_sensor)
    // R_sensor = (V / I) - R_series
    float totalResistance = (busVoltage_V * 1000.0) / current_mA;  // Convert V to mV
    float sensorResistance = totalResistance - SERIES_RESISTOR - SHUNT_RESISTOR;

    // Clamp to valid range
    if (sensorResistance < SENSOR_R_MIN) {
        sensorResistance = SENSOR_R_MIN;
    }
    if (sensorResistance > SENSOR_R_MAX) {
        sensorResistance = SENSOR_R_MAX;
    }

    // Linear interpolation: resistance to pressure
    float pressure = PRESSURE_MIN +
        (sensorResistance - SENSOR_R_MIN) *
        (PRESSURE_MAX - PRESSURE_MIN) /
        (SENSOR_R_MAX - SENSOR_R_MIN);

    return pressure;
}

/**
 * Apply moving average filter
 */
float applyFilter(float newValue) {
    pressureBuffer[bufferIndex] = newValue;
    bufferIndex = (bufferIndex + 1) % FILTER_SAMPLES;

    if (bufferIndex == 0) {
        bufferFilled = true;
    }

    // Calculate average
    int samples = bufferFilled ? FILTER_SAMPLES : bufferIndex;
    if (samples == 0) return newValue;

    float sum = 0.0;
    for (int i = 0; i < samples; i++) {
        sum += pressureBuffer[i];
    }

    return sum / samples;
}

/**
 * Main reading function - call this periodically
 * Updates global oilPressureBar, oilPressurePsi, and oilPressureStatus
 */
void readOilPressure() {
    if (!oilPressureSensorActive) {
        oilPressureStatus = 3;  // Error
        return;
    }

    // Read current
    float current_mA = readOilPressureCurrentRaw();

    // Check for errors
    if (current_mA < 0) {
        oilPressureStatus = 3;  // Sensor error
        return;
    }

    // Check for circuit faults
    if (current_mA < CURRENT_MIN_VALID) {
        Serial.println(F("OilP: Open circuit detected"));
        oilPressureStatus = 3;
        return;
    }

    if (current_mA > CURRENT_MAX_VALID) {
        Serial.println(F("OilP: Short circuit detected"));
        oilPressureStatus = 3;
        return;
    }

    // Read bus voltage for accurate calculation
    float busVoltage_V = ina226_oil.getBusVoltage_V();

    // Convert to pressure
    float rawPressure = currentToPressure(current_mA, busVoltage_V);

    if (rawPressure < 0) {
        oilPressureStatus = 3;
        return;
    }

    // Apply filter
    oilPressureBar = applyFilter(rawPressure);
    oilPressurePsi = oilPressureBar * 14.5038;  // Convert Bar to PSI

    // Update status
    if (oilPressureBar < PRESSURE_CRITICAL) {
        oilPressureStatus = 2;  // Critical
    } else if (oilPressureBar < PRESSURE_WARNING) {
        oilPressureStatus = 1;  // Warning
    } else {
        oilPressureStatus = 0;  // OK
    }
}

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * Get pressure reading as formatted string
 */
void getOilPressureString(char* buffer, int bufferSize) {
    if (oilPressureStatus == 3) {
        snprintf(buffer, bufferSize, "ERR");
    } else {
        // Format: "X.X Bar" or "XX PSI"
        dtostrf(oilPressureBar, 3, 1, buffer);
        strncat(buffer, "B", bufferSize - strlen(buffer) - 1);
    }
}

/**
 * Print diagnostic information to Serial
 */
void printOilPressureDiagnostics() {
    if (!oilPressureSensorActive) {
        Serial.println(F("Oil pressure sensor not active"));
        return;
    }

    float current_mA = ina226_oil.getCurrent_mA();
    float busVoltage_V = ina226_oil.getBusVoltage_V();
    float shuntVoltage_mV = ina226_oil.getShuntVoltage_mV();

    Serial.println(F("--- Oil Pressure Diagnostics ---"));
    Serial.print(F("Bus Voltage:    ")); Serial.print(busVoltage_V); Serial.println(F(" V"));
    Serial.print(F("Shunt Voltage:  ")); Serial.print(shuntVoltage_mV); Serial.println(F(" mV"));
    Serial.print(F("Current:        ")); Serial.print(current_mA); Serial.println(F(" mA"));
    Serial.print(F("Pressure:       ")); Serial.print(oilPressureBar); Serial.println(F(" Bar"));
    Serial.print(F("Pressure:       ")); Serial.print(oilPressurePsi); Serial.println(F(" PSI"));
    Serial.print(F("Status:         "));
    switch (oilPressureStatus) {
        case 0: Serial.println(F("OK")); break;
        case 1: Serial.println(F("WARNING")); break;
        case 2: Serial.println(F("CRITICAL")); break;
        case 3: Serial.println(F("ERROR")); break;
    }
    Serial.println(F("--------------------------------"));
}

// ============================================================================
// Calibration (Optional)
// ============================================================================

/**
 * Run calibration routine
 * Call with sensor disconnected for zero current baseline,
 * then with known pressure applied
 */
void calibrateOilPressure() {
    Serial.println(F("Oil Pressure Calibration"));
    Serial.println(F("========================"));
    Serial.println(F("1. Disconnect sensor and press any key..."));

    // Wait for input
    while (!Serial.available()) delay(10);
    while (Serial.available()) Serial.read();

    float openCurrent = ina226_oil.getCurrent_mA();
    Serial.print(F("Open circuit current: "));
    Serial.print(openCurrent);
    Serial.println(F(" mA (should be ~0)"));

    Serial.println(F("2. Connect sensor with 0 Bar and press any key..."));
    while (!Serial.available()) delay(10);
    while (Serial.available()) Serial.read();

    float zeroCurrent = ina226_oil.getCurrent_mA();
    Serial.print(F("Zero pressure current: "));
    Serial.print(zeroCurrent);
    Serial.println(F(" mA"));

    Serial.println(F("3. Apply 10 Bar and press any key..."));
    while (!Serial.available()) delay(10);
    while (Serial.available()) Serial.read();

    float maxCurrent = ina226_oil.getCurrent_mA();
    Serial.print(F("Max pressure current: "));
    Serial.print(maxCurrent);
    Serial.println(F(" mA"));

    Serial.println(F("\nCalibration complete. Use these values to adjust constants."));
}
```

## Integration with Event System

To integrate with the EngineMonitor event-driven system:

```cpp
// In main.cpp or dedicated sensor module

#include "OilPressureSensor.h"  // Put above code in library

void handleClockTimeEvent(int event, int param) {
    // ... other sensor reads ...

    readOilPressure();

    // Update display
    char buffer[8];
    getOilPressureString(buffer, sizeof(buffer));

    // Format for display (e.g., "OIL: 3.2B OK")
    char displayLine[17];
    snprintf(displayLine, sizeof(displayLine), "OIL:%s %s",
        buffer,
        oilPressureStatus == 0 ? "OK" :
        oilPressureStatus == 1 ? "LO" :
        oilPressureStatus == 2 ? "!!" : "ER");

    displayTextToScreen(SCR_ENGINE, 0, displayLine);

    // Store in SensorData array
    SensorData[SENSOR_OIL_PRESSURE] = (int)(oilPressureBar * 10);  // Store as 0.1 Bar
}
```

## Troubleshooting

| Symptom | Possible Cause | Solution |
|---------|---------------|----------|
| Constant "ERR" reading | I2C communication failure | Check wiring, pull-ups, address conflicts |
| Reading stuck at 0 Bar | Open circuit | Check sensor ground connection |
| Reading stuck at 10 Bar | Short circuit | Check for pinched wires |
| Noisy/jumping readings | Electrical interference | Add more filtering, check ground loop |
| Slow response | Filter too aggressive | Reduce FILTER_SAMPLES |
| INA226 not found | Wrong I2C address | Check module address jumpers |

## Safety Considerations

1. **Never rely solely on this system** - Always use backup mechanical gauges for critical flight instruments
2. **Test warning thresholds** on ground before flight
3. **Monitor for sensor drift** - Calibrate periodically
4. **Fail-safe design** - System shows "ERR" rather than false readings on faults
5. **Heat management** - Keep electronics away from engine heat sources

## References

- [VDO Pressure Sender Datasheet](https://www.vdo.com/catalog/en/Products/Pressure/Pressure-Senders/)
- [INA226 Datasheet](https://www.ti.com/product/INA226)
- [INA226_WE Library](https://github.com/wollewald/INA226_WE)
