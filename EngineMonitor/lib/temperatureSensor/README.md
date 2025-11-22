# TemperatureSensor Module

Static module for reading MAX6675 thermocouple-to-digital converter sensors with calibration support.

## Features

- **Static interface** - No object instantiation required
- **Multiple sensor support** - Manage up to 8 MAX6675 sensors simultaneously
- **Calibration support** - Temperature offset and scaling factor
- **Extended range** - Support for -20°C to 800°C (native MAX6675 is 0-1024°C)
- **Error detection** - Detects thermocouple open circuit conditions
- **SPI configuration** - Automatic SPI initialization with configurable clock

## Hardware

- **Device**: MAX6675 Cold-Junction-Compensated K-Thermocouple-to-Digital Converter
- **Resolution**: 0.25°C (12-bit)
- **Accuracy**: ±2°C typical
- **Conversion time**: 220ms typical
- **Interface**: SPI (Mode 0, MSB first)

## Usage

### Basic Usage

```cpp
#include <TemperatureSensor.h>
#include <PinsMap.h>

void setup() {
    Serial.begin(9600);

    // Initialize SPI bus (optional - automatically done in begin())
    TemperatureSensor::initializeSPI();

    // Configure sensors with offsets
    // Ambient temp: -20°C offset for negative temperature range
    TemperatureSensor::begin(PIN_MAX6675_AMBIENT, -20.0);

    // EGT: No offset needed (always positive)
    TemperatureSensor::begin(PIN_MAX6675_EGT, 0.0);

    // Oil temp: No offset
    TemperatureSensor::begin(PIN_MAX6675_OIL, 0.0);

    // CHT: No offset
    TemperatureSensor::begin(PIN_MAX6675_CHT1, 0.0);
}

void loop() {
    // Read temperatures
    float ambientTemp = TemperatureSensor::readTemperature(PIN_MAX6675_AMBIENT);
    float egtTemp = TemperatureSensor::readTemperature(PIN_MAX6675_EGT);
    float oilTemp = TemperatureSensor::readTemperature(PIN_MAX6675_OIL);
    float chtTemp = TemperatureSensor::readTemperature(PIN_MAX6675_CHT1);

    // Check for errors
    if (TemperatureSensor::hasError(PIN_MAX6675_AMBIENT)) {
        Serial.println(F("Ambient sensor error - thermocouple disconnected?"));
    } else {
        Serial.print(F("Ambient: "));
        Serial.print(ambientTemp);
        Serial.println(F("°C"));
    }

    if (!TemperatureSensor::hasError(PIN_MAX6675_EGT)) {
        Serial.print(F("EGT: "));
        Serial.print(egtTemp);
        Serial.println(F("°C"));
    }

    // Wait for next conversion (220ms minimum)
    delay(250);
}
```

### Advanced Usage with Calibration

```cpp
void setup() {
    // Configure sensor with offset AND calibration factor
    // Example: Sensor reads 5% high, so use factor of 0.95
    // and has -20°C offset
    TemperatureSensor::begin(PIN_MAX6675_AMBIENT, -20.0, 0.95);

    // Later, update calibration if needed
    TemperatureSensor::setTemperatureOffset(PIN_MAX6675_AMBIENT, -18.5);
}

void loop() {
    float temp = TemperatureSensor::readTemperature(PIN_MAX6675_AMBIENT);

    if (temp != TemperatureSensor::INVALID_TEMPERATURE) {
        // Check how long since last successful read
        uint32_t timeSinceRead = TemperatureSensor::getTimeSinceLastRead(PIN_MAX6675_AMBIENT);

        Serial.print(F("Temperature: "));
        Serial.print(temp, 2);  // 2 decimal places
        Serial.print(F("°C (read "));
        Serial.print(timeSinceRead);
        Serial.println(F("ms ago)"));
    }

    delay(250);
}
```

### Error Handling

```cpp
void readSensor(uint8_t csPin, const char* sensorName) {
    if (!TemperatureSensor::isInitialized(csPin)) {
        Serial.print(sensorName);
        Serial.println(F(" - NOT INITIALIZED"));
        return;
    }

    float temp = TemperatureSensor::readTemperature(csPin);

    if (TemperatureSensor::hasError(csPin)) {
        Serial.print(sensorName);
        Serial.println(F(" - ERROR: Thermocouple disconnected"));
    } else if (temp == TemperatureSensor::INVALID_TEMPERATURE) {
        Serial.print(sensorName);
        Serial.println(F(" - ERROR: Invalid reading"));
    } else if (temp < TemperatureSensor::MIN_TEMPERATURE ||
               temp > TemperatureSensor::MAX_TEMPERATURE) {
        Serial.print(sensorName);
        Serial.print(F(" - WARNING: Out of range: "));
        Serial.println(temp);
    } else {
        Serial.print(sensorName);
        Serial.print(F(": "));
        Serial.print(temp, 2);
        Serial.println(F("°C"));
    }
}

void loop() {
    readSensor(PIN_MAX6675_AMBIENT, "Ambient");
    readSensor(PIN_MAX6675_EGT, "EGT");
    readSensor(PIN_MAX6675_OIL, "Oil");
    readSensor(PIN_MAX6675_CHT1, "CHT1");

    delay(250);
}
```

## API Reference

### Initialization

#### `initializeSPI(uint8_t spiClockDivider = SPI_CLOCK_DIV4)`
Initialize the SPI bus. Called automatically by `begin()` if not already initialized.
- **spiClockDivider**: SPI clock divider (default: SPI_CLOCK_DIV4 = 4MHz on 16MHz Arduino)

#### `begin(uint8_t csPin, float temperatureOffset = 0.0, float calibrationFactor = 1.0)`
Configure a MAX6675 sensor.
- **csPin**: Chip select pin for the sensor
- **temperatureOffset**: Offset added to raw reading (e.g., -20.0 for negative temps)
- **calibrationFactor**: Multiplier for calibration (default: 1.0)
- **Returns**: true if successful, false if no space available

### Reading

#### `readTemperature(uint8_t csPin)`
Read calibrated temperature from sensor.
- **csPin**: Chip select pin
- **Returns**: Temperature in °C, or `INVALID_TEMPERATURE` (-999.0) if error

#### `readRawData(uint8_t csPin)`
Read raw 16-bit data from MAX6675.
- **csPin**: Chip select pin
- **Returns**: Raw 16-bit value

### Status & Diagnostics

#### `hasError(uint8_t csPin)`
Check if last read had an error.
- **Returns**: true if thermocouple disconnected or error occurred

#### `isInitialized(uint8_t csPin)`
Check if sensor is configured.
- **Returns**: true if sensor is initialized

#### `getTimeSinceLastRead(uint8_t csPin)`
Get time since last successful read.
- **Returns**: Milliseconds since last read, or 0 if never read

### Configuration

#### `getTemperatureOffset(uint8_t csPin)`
Get configured temperature offset.
- **Returns**: Offset in °C, or 0.0 if sensor not found

#### `setTemperatureOffset(uint8_t csPin, float offset)`
Update temperature offset.
- **offset**: New offset in °C
- **Returns**: true if successful, false if sensor not found

## Constants

- `MIN_TEMPERATURE` = -20.0°C
- `MAX_TEMPERATURE` = 800.0°C
- `INVALID_TEMPERATURE` = -999.0°C
- `MAX6675_CONVERSION_TIME_MS` = 220ms

## Technical Notes

### Temperature Calculation

The MAX6675 provides 12 bits of temperature data (bits 3-14 of the 16-bit word):
- Resolution: 0.25°C per bit
- Native range: 0°C to 1023.75°C
- Calibrated temp = (raw_temp × calibration_factor) + offset

### Error Detection

Bit 2 of the 16-bit word indicates thermocouple status:
- 0 = Thermocouple connected
- 1 = Thermocouple open circuit (disconnected)

### Timing

The MAX6675 requires ~220ms to complete a temperature conversion. Reading too frequently will return stale data. Recommended minimum interval: 250ms.

### SPI Configuration

- Mode: SPI_MODE0 (CPOL=0, CPHA=0)
- Bit order: MSB first
- Clock: 4MHz typical (SPI_CLOCK_DIV4 on 16MHz Arduino)
- Max clock: 4.3MHz (from datasheet)

## Memory Usage

- Each sensor configuration: ~12 bytes
- Maximum 8 sensors: ~96 bytes RAM
- Code size: ~1-2KB flash (depends on usage)

## License

This module is part of the N2 Arduino Engine Monitor System.
