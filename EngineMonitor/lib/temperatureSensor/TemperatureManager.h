// TemperatureManager - Manages multiple MAX31855 temperature sensors
// Provides centralized sensor configuration and data collection
//
// Returns sensor data as a collection with sensor ID, temperature value, and success flag

#ifndef TEMPERATURE_MANAGER_H
#define TEMPERATURE_MANAGER_H

#include <Arduino.h>
#include <EEPROM.h>
#include "TemperatureSensor.h"
#include "../systemConfig/PinsMap.h"
#include "../systemConfig/EMSMemoryMap.h"
#include "../common/SensorReading.h"
#include <SensorTypes.h>

namespace TemperatureManager {

// ========================================
// DATA STRUCTURES
// ========================================

/**
 * Structure to hold temperature sensor reading result
 * Extends Common::SensorReading with temperature-specific semantics
 *
 * Field mappings:
 *   value   = temperature in whole degrees Celsius
 *   warning = true if temperature exceeds over-temp warning threshold
 */
struct SensorReading : public Common::SensorReading {
    // Inherits: id, value (celsius), status, warning (overTemperature), success
};

// Maximum number of temperature sensors in the system
const uint8_t MAX_TEMP_SENSORS = 8;

// ========================================
// SENSOR CONFIGURATION
// ========================================

/**
 * Internal sensor configuration structure
 */
struct SensorConfig {
    uint8_t sensorId;         // Sensor type ID from SensorTypes.h
    uint8_t chipSelectPin;    // CS pin for this sensor
    float temperatureOffset;  // Calibration offset in °C
    const char* name;         // Sensor name for debugging
};

// Internal sensor configuration array
namespace Internal {
    /**
     * Get EEPROM address for temperature sensor calibration offset
     * @param sensorId Sensor type ID from SensorTypes.h
     * @return EEPROM address for calibration offset, or 0 if not found
     */
    static uint16_t getCalibrationAddress(uint8_t sensorId) {
        switch (sensorId) {
            case SENSOR_TEMPERATURE_AMB:   return EEPROM_CAL_TEMP_AMB_OFFSET;
            case SENSOR_TEMPERATURE_EGT_1: return EEPROM_CAL_TEMP_EGT_OFFSET;
            case SENSOR_TEMPERATURE_CHT_1: return EEPROM_CAL_TEMP_CHT1_OFFSET;
            case SENSOR_TEMPERATURE_CHT_2: return EEPROM_CAL_TEMP_CHT2_OFFSET;
            case SENSOR_TEMPERATURE_CHT_3: return EEPROM_CAL_TEMP_CHT3_OFFSET;
            case SENSOR_TEMPERATURE_CHT_4: return EEPROM_CAL_TEMP_CHT4_OFFSET;
            case SENSOR_TEMPERATURE_OIL:   return EEPROM_CAL_TEMP_OIL_OFFSET;
            default: return 0;
        }
    }

    /**
     * Read temperature offset from EEPROM
     * @param sensorId Sensor type ID from SensorTypes.h
     * @return Temperature offset in degrees Celsius
     */
    static float readOffsetFromEeprom(uint8_t sensorId) {
        uint16_t address = getCalibrationAddress(sensorId);
        if (address == 0) {
            return 0.0f;
        }

        // Read int16_t value (stored as degrees * 10)
        int16_t rawValue;
        EEPROM.get(address, rawValue);

        // Convert to float (divide by 10)
        return rawValue / 10.0f;
    }

    static SensorConfig sensorConfigs[] = {
        { SENSOR_TEMPERATURE_AMB,   PIN_TEMP_ENGINE,   0.0,  "Engine" },
        { SENSOR_TEMPERATURE_EGT_1, PIN_TEMP_EGT,      0.0,  "EGT"     },
        { SENSOR_TEMPERATURE_CHT_1, PIN_TEMP_CHT1,     0.0,  "CHT1"    },
        { SENSOR_TEMPERATURE_OIL,   PIN_TEMP_OIL,      0.0,  "OIL"    },
#ifdef PIN_TEMP_CHT2
        { SENSOR_TEMPERATURE_CHT_2, PIN_TEMP_CHT2,     0.0,  "CHT2"    },
#endif
#ifdef PIN_TEMP_CHT3
        { SENSOR_TEMPERATURE_CHT_3, PIN_TEMP_CHT3,     0.0,  "CHT3"    },
#endif
    };

    static const uint8_t sensorCount = sizeof(sensorConfigs) / sizeof(SensorConfig);
}

// ========================================
// PUBLIC API
// ========================================

/**
 * Initialize all temperature sensors
 * Must be called once during setup before reading sensors
 *
 * @return Number of sensors successfully initialized
 */
static uint8_t initialize() {
    uint8_t successCount = 0;

    // Configure each sensor
    for (uint8_t i = 0; i < Internal::sensorCount; i++) {
        SensorConfig* config = &Internal::sensorConfigs[i];

        // Read calibration offset from EEPROM
        config->temperatureOffset = Internal::readOffsetFromEeprom(config->sensorId);

        if (TemperatureSensor::begin(config->chipSelectPin, config->temperatureOffset)) {
            successCount++;
        }
    }

    return successCount;
}

/**
 * Read all temperature sensors and return results as an array
 *
 * @param readings Array to store sensor readings (must be at least MAX_TEMP_SENSORS in size)
 * @param maxReadings Maximum number of readings to store
 * @return Number of sensors read (successful or not)
 */
static uint8_t readAll(Common::SensorReading* readings, uint8_t startIndex, uint8_t maxReadings) {
    uint8_t readCount = startIndex;

    for (uint8_t i = 0; i < Internal::sensorCount && readCount < maxReadings; i++) {
        SensorConfig* config = &Internal::sensorConfigs[i];

        // Read temperature
        float temperature = TemperatureSensor::readTemperature(config->chipSelectPin);
        bool hasError = TemperatureSensor::hasError(config->chipSelectPin);
        bool overheated = TemperatureSensor::isOverheated(config->chipSelectPin);
        int8_t status = TemperatureSensor::getStatus(config->chipSelectPin);

        // Store result
        readings[readCount].id = config->sensorId;
        readings[readCount].status = status;
        readings[readCount].value = (int16_t)round(temperature);
        readings[readCount].warning = overheated;
        readings[readCount].success = !hasError && (temperature != TemperatureSensor::INVALID_TEMPERATURE);

        readCount++;
    }

    return readCount;
}

/**
 * Read a specific sensor by sensor ID
 *
 * @param sensorId Sensor ID from SensorTypes.h
 * @param reading Pointer to SensorReading structure to store result
 * @return true if sensor was found and read, false if sensor ID not found
 */
inline bool readSensor(uint8_t sensorId, SensorReading* reading) {
    // Find sensor configuration
    for (uint8_t i = 0; i < Internal::sensorCount; i++) {
        SensorConfig* config = &Internal::sensorConfigs[i];

        if (config->sensorId == sensorId) {
            // Read temperature
            float temperature = TemperatureSensor::readTemperature(config->chipSelectPin);
            bool hasError = TemperatureSensor::hasError(config->chipSelectPin);

            // Store result
            reading->id = config->sensorId;
            reading->value = (int16_t)round(temperature);
            reading->success = !hasError && (temperature != TemperatureSensor::INVALID_TEMPERATURE);

            return true;
        }
    }

    return false;  // Sensor ID not found
}

/**
 * Get the number of configured sensors
 *
 * @return Number of sensors
 */
static uint8_t getSensorCount() {
    return Internal::sensorCount;
}

/**
 * Get sensor configuration by index
 *
 * @param index Sensor index (0 to getSensorCount()-1)
 * @return Pointer to sensor config, or nullptr if index out of range
 */
static const SensorConfig* getSensorConfig(uint8_t index) {
    if (index >= Internal::sensorCount) {
        return nullptr;
    }
    return &Internal::sensorConfigs[index];
}

} // namespace TemperatureManager

#endif // TEMPERATURE_MANAGER_H
