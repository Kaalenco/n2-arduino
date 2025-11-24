// OilPressureManager - Manages VDO oil pressure sensor via INA226
// Provides centralized sensor configuration and data collection
//
// Returns sensor data with pressure value and status flags

#ifndef OIL_PRESSURE_MANAGER_H
#define OIL_PRESSURE_MANAGER_H

#include <Arduino.h>
#include <EEPROM.h>
#include "OilPressureSensor.h"
#include "../systemConfig/EMSMemoryMap.h"
#include "../common/SensorReading.h"
#include <SensorTypes.h>

namespace OilPressureManager {

// ========================================
// DATA STRUCTURES
// ========================================

/**
 * Structure to hold oil pressure sensor reading result
 * Extends Common::SensorReading with pressure-specific fields
 *
 * Field mappings:
 *   value   = pressure in Bar * 10 (0-100 for 0-10 Bar)
 *   warning = true if pressure is below warning threshold
 */
struct SensorReading : public Common::SensorReading {
    // Inherits: id, value (pressureBarX10), status, warning (underPressure), success
};

// ========================================
// SENSOR CONFIGURATION
// ========================================

namespace Internal {
    // I2C address for INA226 oil pressure sensor
    static const uint8_t I2C_ADDRESS = 0x40;

    // Initialized flag
    static bool initialized = false;

    // Last reading
    static SensorReading lastReading;

    /**
     * Read calibration zero offset from EEPROM
     * @return Zero offset ADC value
     */
    static int16_t readZeroOffsetFromEeprom() {
        int16_t value;
        EEPROM.get(EEPROM_CAL_OIL_PRESS_ZERO, value);
        return value;
    }

    /**
     * Read calibration scale factor from EEPROM
     * @return Scale factor (counts per PSI * 10)
     */
    static int16_t readScaleFactorFromEeprom() {
        int16_t value;
        EEPROM.get(EEPROM_CAL_OIL_PRESS_SCALE, value);
        return value;
    }

    /**
     * Read low pressure warning threshold from EEPROM
     * @return Warning threshold in PSI * 10
     */
    static int16_t readWarningThresholdFromEeprom() {
        int16_t value;
        EEPROM.get(EEPROM_PRESS_OIL_WARNING_LOW, value);
        return value;
    }
}

// ========================================
// PUBLIC API
// ========================================

/**
 * Initialize the oil pressure sensor
 * Must be called once during setup before reading sensor
 *
 * @return true if sensor was successfully initialized
 */
static bool initialize() {
    // Initialize the low-level sensor
    if (!OilPressureSensor::begin(Internal::I2C_ADDRESS)) {
        Internal::initialized = false;
        return false;
    }

    // Initialize last reading
    Internal::lastReading.id = SENSOR_PRESSURE_OIL;
    Internal::lastReading.value = 0;
    Internal::lastReading.status = OilPressureSensor::OIL_STATUS_OK;
    Internal::lastReading.warning = false;
    Internal::lastReading.success = false;

    Internal::initialized = true;
    return true;
}

/**
 * Read the oil pressure sensor and return result
 *
 * @param reading Pointer to SensorReading structure to store result
 * @return true if read was successful
 */
static uint8_t read(Common::SensorReading* readings, uint8_t index) {
    Common::SensorReading* reading = &readings[index];
    
    if (!Internal::initialized) {
        reading->id = SENSOR_PRESSURE_OIL;
        reading->value = 0;
        reading->status = OilPressureSensor::OIL_STATUS_NOT_INIT;
        reading->warning = false;
        reading->success = false;
        return index+1;
    }

    // Read from sensor (updates internal values)
    OilPressureSensor::readPressure();

    // Populate reading structure
    reading->id = SENSOR_PRESSURE_OIL;
    reading->value = OilPressureSensor::getAveragePressureX10();
    reading->status = OilPressureSensor::getStatus();
    reading->warning = OilPressureSensor::isUnderPressure();
    reading->success = !OilPressureSensor::hasError();

    // Store last reading with extended fields
    Internal::lastReading.id = reading->id;
    Internal::lastReading.value = reading->value;
    Internal::lastReading.status = reading->status;
    Internal::lastReading.warning = reading->warning;
    Internal::lastReading.success = reading->success;

    return index+1;
}

/**
 * Get the last reading without triggering a new read
 *
 * @return Pointer to last SensorReading
 */
static const SensorReading* getLastReading() {
    return &Internal::lastReading;
}

/**
 * Get the average pressure as integer (Bar * 10)
 * Useful for CAN bus packing
 *
 * @return Pressure in Bar * 10
 */
static uint8_t getAveragePressureX10() {
    return OilPressureSensor::getAveragePressureX10();
}

/**
 * Check if pressure is below warning threshold
 *
 * @return true if under pressure warning
 */
static bool isUnderPressure() {
    return OilPressureSensor::isUnderPressure();
}

/**
 * Get current sensor status
 *
 * @return Status code
 */
static uint8_t getStatus() {
    return OilPressureSensor::getStatus();
}

/**
 * Check if sensor is initialized
 *
 * @return true if initialized
 */
static bool isInitialized() {
    return Internal::initialized && OilPressureSensor::isInitialized();
}

/**
 * Check if sensor has an error
 *
 * @return true if error
 */
static bool hasError() {
    return OilPressureSensor::hasError();
}

/**
 * Reset the moving average filter
 */
static void resetFilter() {
    OilPressureSensor::resetFilter();
}

} // namespace OilPressureManager

#endif // OIL_PRESSURE_MANAGER_H
