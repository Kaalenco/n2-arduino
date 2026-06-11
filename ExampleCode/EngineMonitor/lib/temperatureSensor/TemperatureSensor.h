// TemperatureSensor - Static module for reading MAX31855 thermocouple sensors
// Provides calibrated temperature readings with offset support for negative temperatures
// Range: -20°C to 800°C (MAX31855 native range is 0-1024°C, calibrated with offset)
//
// Usage:
//   TemperatureSensor::begin(CS_PIN, -20.0);  // Initialize with -20°C offset
//   float temp = TemperatureSensor::readTemperature(CS_PIN);
//   if (!TemperatureSensor::hasError(CS_PIN)) {
//     // Use temperature...
//   }

#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

#include <Arduino.h>
#include <MAX31855.h>
#include "../systemConfig/PinsMap.h"  // For PIN_SPI_CS definition

namespace TemperatureSensor {

// ========================================
// CONSTANTS
// ========================================
const float MIN_TEMPERATURE = -20.0;   // Minimum supported temperature (°C)
const float MAX_TEMPERATURE = 800.0;   // Maximum supported temperature (°C)
const float INVALID_TEMPERATURE = -999.0;  // Error value

// MAX31855 timing constants (from datasheet)
const uint16_t MAX31855_CONVERSION_TIME_MS = 220;  // Conversion time (220ms typical)
const uint16_t MAX31855_READ_DELAY_US = 1;         // Delay between CS and read

// MAX31855 bit definitions
const uint16_t MAX31855_ERROR_BIT = 0x04;          // Bit 2: Thermocouple open circuit
const uint16_t MAX31855_DEVICE_ID_BIT = 0x02;      // Bit 1: Device ID (always 0)
const uint16_t MAX31855_DUMMY_BIT = 0x01;          // Bit 0: Dummy sign bit

// ========================================
// CONFIGURATION STRUCTURE
// ========================================
struct SensorConfig {
    uint8_t chipSelectPin;      // CS pin for this sensor
    float temperatureOffset;    // Calibration offset (e.g., -20.0 for negative range)
    float overTempWarning;      // Over-temperature warning threshold
    uint32_t lastReadTime;      // Timestamp of last successful read
    bool initialized;           // Initialization flag
    uint8_t status;              // Last status code
    double lastValue;           // Last temperature value
    MAX31855* sensorInstance;    // Pointer to MAX31855 instance
};

// Maximum number of sensors that can be configured
const uint8_t MAX_SENSORS = 8;

// ========================================
// INTERNAL STATE
// ========================================
namespace Internal {
    // Array to store sensor configurations
    static SensorConfig sensors[MAX_SENSORS];
    static uint8_t sensorCount = 0;

    // Find sensor config by CS pin
    static SensorConfig* findSensor(uint8_t csPin) {
        for (uint8_t i = 0; i < sensorCount; i++) {
            if (sensors[i].chipSelectPin == csPin) {
                return &sensors[i];
            }
        }
        return nullptr;
    }
}

// ========================================
// PUBLIC API
// ========================================

/**
 * Configure a MAX31855 sensor
 *
 * @param csPin Chip select pin for this sensor
 * @param temperatureOffset Offset to add to raw reading (e.g., -20.0 for sensors measuring below 0°C)
 * @param calibrationFactor Multiplier for calibration (default: 1.0)
 * @return true if sensor was configured successfully, false if no space available
 */
static bool begin(uint8_t csPin, float temperatureOffset = 0.0, float overTemperature = 100.0) {
    // Check if sensor already configured
    SensorConfig* existing = Internal::findSensor(csPin);
    if (existing != nullptr) {
        // Update existing configuration
        existing->temperatureOffset = temperatureOffset;
        existing->initialized = true;
        return true;
    }

    // Add new sensor if space available
    if (Internal::sensorCount >= MAX_SENSORS) {
        return false;  // No space for more sensors
    }

    // Configure new sensor
    SensorConfig* config = &Internal::sensors[Internal::sensorCount++];
    config->chipSelectPin = csPin;
    config->temperatureOffset = temperatureOffset;
    config->overTempWarning = overTemperature;
    config->lastReadTime = 0;
    config->initialized = true;
    config->lastValue = 0.0;
    config->status = 4; // Default to error until first read

    // Create MAX31855 instance: CS, DO, CLK
    config->sensorInstance = new MAX31855(config->chipSelectPin, thermoDO, thermoCLK);

    MAX31855& tc = *(config->sensorInstance);

    tc.begin();
    tc.setOffset(temperatureOffset);
    tc.setSPIspeed(4000000);
    tc.read();
    
    config->status = tc.getStatus();
    return true;
}

static double readTemperature(uint8_t csPin) {
    SensorConfig* config = Internal::findSensor(csPin);
    if (config == nullptr) {
        return INVALID_TEMPERATURE;  // Sensor not configured
    }

    // Read temperature from sensor
    // wait for conversion time
    if (millis() - config->lastReadTime < MAX31855_CONVERSION_TIME_MS) {
        delay(MAX31855_CONVERSION_TIME_MS - (millis() - config->lastReadTime));
    }

    MAX31855& tc = *(config->sensorInstance);
    // read raw data from sensor
    tc.read();
    // update config status and last value
    config->status = tc.getStatus();
    config->lastValue = tc.getTemperature();
    config->lastReadTime = millis();
    return config->lastValue;
}

/**
 * Check if the last read had an error
 *
 * @param csPin Chip select pin for the sensor to check
 * @return true if error occurred (thermocouple disconnected), false otherwise
 */
static bool hasError(uint8_t csPin) {
    SensorConfig* config = Internal::findSensor(csPin);
    if (config == nullptr) {
        return true;  // Sensor not configured
    }
    return config->status != 0  ;  // Non-zero status indicates error

// STATUS_ERROR	4	        Thermocouple short to VCC	check wiring	
// STATUS_NOREAD	        128	No read done yet	check wiring	only before first read()
// STATUS_NO_COMMUNICATION	129	No communication	check wiring
}

static uint8_t getStatus(uint8_t csPin) {
    SensorConfig* config = Internal::findSensor(csPin);
    if (config == nullptr) {
        return 255;  // Sensor not configured
    }
    return config->status;
}
/**
 * Check if sensor temperature exceeds over-temperature threshold
 *
 * @param csPin Chip select pin for the sensor to check
 * @return true if temperature exceeds threshold, false otherwise
 */
static bool isOverheated(uint8_t csPin) {
    SensorConfig* config = Internal::findSensor(csPin);
    if (config == nullptr) {
        return false;  // Sensor not configured
    }
    return config->lastValue > config->overTempWarning;
}

/**
 * Check if sensor is initialized
 *
 * @param csPin Chip select pin for the sensor to check
 * @return true if sensor is configured and initialized
 */
inline bool isInitialized(uint8_t csPin) {
    SensorConfig* config = Internal::findSensor(csPin);
    return (config != nullptr && config->initialized);
}

/**
 * Get the configured temperature offset for a sensor
 *
 * @param csPin Chip select pin for the sensor
 * @return Temperature offset in °C, or 0.0 if sensor not found
 */
inline float getTemperatureOffset(uint8_t csPin) {
    SensorConfig* config = Internal::findSensor(csPin);
    if (config == nullptr) {
        return 0.0;
    }
    return config->temperatureOffset;
}

/**
 * Update the temperature offset for a sensor
 *
 * @param csPin Chip select pin for the sensor
 * @param offset New temperature offset in °C
 * @return true if successful, false if sensor not found
 */
inline bool setTemperatureOffset(uint8_t csPin, float offset) {
    SensorConfig* config = Internal::findSensor(csPin);
    if (config == nullptr) {
        return false;
    }
    config->temperatureOffset = offset;
    //config->sensorInstance->setOffset(offset);
    return true;
}

/**
 * Get the time since last successful read
 *
 * @param csPin Chip select pin for the sensor
 * @return Milliseconds since last read, or 0 if never read
 */
inline uint32_t getTimeSinceLastRead(uint8_t csPin) {
    SensorConfig* config = Internal::findSensor(csPin);
    if (config == nullptr || config->lastReadTime == 0) {
        return 0;
    }
    return millis() - config->lastReadTime;
}




} // namespace TemperatureSensor

#endif // TEMPERATURE_SENSOR_H
