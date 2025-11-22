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
#include <SensorTypes.h>

namespace OilPressureManager {

// ========================================
// DATA STRUCTURES
// ========================================

/**
 * Structure to hold oil pressure sensor reading result
 */
struct SensorReading {
    uint8_t id;                 // Sensor ID from SensorTypes.h
    uint8_t pressureBarX10;     // Pressure in Bar * 10 (0-100 for 0-10 Bar)
    float pressureBar;          // Pressure in Bar (float)
    float pressurePsi;          // Pressure in PSI (float)
    uint8_t status;             // Status code from OilPressureSensor
    bool underPressure;         // True if pressure is below warning threshold
    bool success;               // True if read was successful, false if error
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
    Internal::lastReading.pressureBarX10 = 0;
    Internal::lastReading.pressureBar = 0.0;
    Internal::lastReading.pressurePsi = 0.0;
    Internal::lastReading.status = OilPressureSensor::STATUS_OK;
    Internal::lastReading.underPressure = false;
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
static bool read(SensorReading* reading) {
    if (!Internal::initialized) {
        reading->id = SENSOR_PRESSURE_OIL;
        reading->pressureBarX10 = 0;
        reading->pressureBar = 0.0;
        reading->pressurePsi = 0.0;
        reading->status = OilPressureSensor::STATUS_NOT_INIT;
        reading->underPressure = false;
        reading->success = false;
        return false;
    }

    // Read from sensor (updates internal values)
    float pressure = OilPressureSensor::readPressure();

    // Populate reading structure
    reading->id = SENSOR_PRESSURE_OIL;
    reading->pressureBar = OilPressureSensor::getAveragePressureBar();
    reading->pressureBarX10 = OilPressureSensor::getAveragePressureX10();
    reading->pressurePsi = OilPressureSensor::getAveragePressurePsi();
    reading->status = OilPressureSensor::getStatus();
    reading->underPressure = OilPressureSensor::isUnderPressure();
    reading->success = !OilPressureSensor::hasError();

    // Store last reading
    Internal::lastReading = *reading;

    return reading->success;
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
 * Get the average pressure in Bar
 * Convenience function for quick access
 *
 * @return Average pressure in Bar
 */
static float getAveragePressureBar() {
    return OilPressureSensor::getAveragePressureBar();
}

/**
 * Get the average pressure in PSI
 * Convenience function for quick access
 *
 * @return Average pressure in PSI
 */
static float getAveragePressurePsi() {
    return OilPressureSensor::getAveragePressurePsi();
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
 * Get sensor name
 *
 * @return Sensor name string
 */
static const char* getSensorName() {
    return "OilPress";
}

/**
 * Print diagnostic information to Serial
 */
static void printDiagnostics() {
    OilPressureSensor::printDiagnostics();
}

/**
 * Reset the moving average filter
 */
static void resetFilter() {
    OilPressureSensor::resetFilter();
}

} // namespace OilPressureManager

#endif // OIL_PRESSURE_MANAGER_H
