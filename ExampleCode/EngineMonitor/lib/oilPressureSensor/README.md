# Oil Pressure Sensor Module

This module provides an interface for reading VDO oil pressure sensors via INA226 current monitor.

## Hardware Setup

- **Sensor**: VDO 360-081-030-015C (0-10 Bar)
- **Interface**: INA226 I2C current/power monitor
- **I2C Address**: 0x40 (default)

See `docs/oil-pressure-sensor.md` for detailed circuit design and wiring.

## Usage

### Basic Usage

```cpp
#include "OilPressureManager.h"

void setup() {
    Wire.begin();

    // Initialize the sensor
    if (OilPressureManager::initialize()) {
        Serial.println(F("Oil pressure sensor initialized"));
    } else {
        Serial.println(F("Oil pressure sensor failed"));
    }
}

void loop() {
    // Read sensor
    OilPressureManager::SensorReading reading;
    if (OilPressureManager::read(&reading)) {
        Serial.print(F("Pressure: "));
        Serial.print(reading.pressureBar);
        Serial.println(F(" Bar"));
    }

    delay(500);
}
```

### Integration with EngineDataLogger

```cpp
#include "OilPressureManager.h"
#include "EngineDataLogger.h"

CanbusLogging::EngineDataLogger engineLogger(PIN_CANBUS);

void readAndLogOilPressure() {
    // Read the oil pressure
    OilPressureManager::SensorReading reading;
    OilPressureManager::read(&reading);

    // Set in engine data logger (uses average values)
    engineLogger.setOilPressure(
        OilPressureManager::getAveragePressureX10(),
        OilPressureManager::isUnderPressure()
    );

    // Add alert if under pressure
    if (OilPressureManager::isUnderPressure()) {
        engineLogger.addAlert(CanbusLogging::ALERT_OIL_PRESSURE_LOW);
    }
}
```

### Quick Access Functions

```cpp
// Get averaged pressure
float bar = OilPressureManager::getAveragePressureBar();
float psi = OilPressureManager::getAveragePressurePsi();
uint8_t x10 = OilPressureManager::getAveragePressureX10();

// Check status
bool warning = OilPressureManager::isUnderPressure();
bool error = OilPressureManager::hasError();
uint8_t status = OilPressureManager::getStatus();
```

## API Reference

### OilPressureManager

| Function | Description |
|----------|-------------|
| `initialize()` | Initialize the sensor. Returns true on success. |
| `read(reading*)` | Read sensor and populate SensorReading struct. |
| `getLastReading()` | Get pointer to last reading without new read. |
| `getAveragePressureBar()` | Get moving average pressure in Bar. |
| `getAveragePressurePsi()` | Get moving average pressure in PSI. |
| `getAveragePressureX10()` | Get pressure as integer (Bar * 10). |
| `isUnderPressure()` | Check if below warning threshold. |
| `hasError()` | Check if sensor has error. |
| `getStatus()` | Get status code. |
| `printDiagnostics()` | Print debug info to Serial. |

### SensorReading Structure

```cpp
struct SensorReading {
    uint8_t id;              // SENSOR_PRESSURE_OIL
    uint8_t pressureBarX10;  // Pressure in Bar * 10
    float pressureBar;       // Pressure in Bar
    float pressurePsi;       // Pressure in PSI
    uint8_t status;          // Status code
    bool underPressure;      // Under pressure warning
    bool success;            // Read success flag
};
```

### Status Codes

| Code | Constant | Description |
|------|----------|-------------|
| 0 | STATUS_OK | Normal operation |
| 1 | STATUS_WARNING | Low pressure warning (< 1.5 Bar) |
| 2 | STATUS_CRITICAL | Critical low pressure (< 0.8 Bar) |
| 3 | STATUS_ERROR | Sensor error (open/short circuit) |
| 4 | STATUS_NOT_INIT | Not initialized |

## Moving Average Filter

The sensor uses an 8-sample moving average to smooth readings. This provides stable values while still being responsive to pressure changes.

- Access averaged value: `getAveragePressureBar()`
- Access instant value: `OilPressureSensor::getCurrentPressureBar()`
- Reset filter: `OilPressureManager::resetFilter()`

## Pressure Thresholds

- **Warning**: < 1.5 Bar
- **Critical**: < 0.8 Bar

These thresholds match the VDO sensor's warning contact specification.

## Dependencies

- `Wire.h` - I2C communication
- `INA226_WE` - INA226 library by Wolfgang Ewald

The INA226_WE library is included in platformio.ini:
```ini
lib_deps =
    wollewald/INA226_WE@^1.3.8
```
