// OilPressureSensor - Static module for reading VDO oil pressure sensor via INA226
// Uses INA226 current monitor to measure current through resistance-based pressure sender
// Pressure range: 0 to 10 Bar (VDO 360-081-030-015C)
//
// Usage:
//   OilPressureSensor::begin(I2C_ADDRESS);
//   OilPressureSensor::readPressure();
//   float avgBar = OilPressureSensor::getAveragePressureBar();

#ifndef OIL_PRESSURE_SENSOR_H
#define OIL_PRESSURE_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include <INA226_WE.h>

namespace OilPressureSensor {

// ========================================
// CONSTANTS
// ========================================

// Default I2C address for INA226
const uint8_t DEFAULT_I2C_ADDRESS = 0x40;

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

// Pressure thresholds
const float PRESSURE_WARNING = 1.5;     // Bar - low pressure warning threshold
const float PRESSURE_CRITICAL = 0.8;    // Bar - critical low pressure

// Filter configuration
const uint8_t FILTER_SAMPLES = 8;       // Number of samples for moving average

// Invalid pressure value
const float INVALID_PRESSURE = -999.0;

// Status codes
const uint8_t STATUS_OK = 0;
const uint8_t STATUS_WARNING = 1;       // Low pressure warning
const uint8_t STATUS_CRITICAL = 2;      // Critical low pressure
const uint8_t STATUS_ERROR = 3;         // Sensor error
const uint8_t STATUS_NOT_INIT = 4;      // Not initialized

// ========================================
// SENSOR STATE
// ========================================
namespace Internal {
    static INA226_WE* ina226 = nullptr;
    static uint8_t i2cAddress = DEFAULT_I2C_ADDRESS;
    static bool initialized = false;

    // Current readings
    static float currentPressureBar = 0.0;
    static float averagePressureBar = 0.0;
    static uint8_t status = STATUS_NOT_INIT;

    // Filter buffer for moving average
    static float pressureBuffer[FILTER_SAMPLES];
    static uint8_t bufferIndex = 0;
    static bool bufferFilled = false;

    // Last read time
    static uint32_t lastReadTime = 0;

    /**
     * Apply moving average filter
     */
    static float applyFilter(float newValue) {
        pressureBuffer[bufferIndex] = newValue;
        bufferIndex = (bufferIndex + 1) % FILTER_SAMPLES;

        if (bufferIndex == 0) {
            bufferFilled = true;
        }

        // Calculate average
        uint8_t samples = bufferFilled ? FILTER_SAMPLES : bufferIndex;
        if (samples == 0) return newValue;

        float sum = 0.0;
        for (uint8_t i = 0; i < samples; i++) {
            sum += pressureBuffer[i];
        }

        return sum / samples;
    }

    /**
     * Convert current reading to pressure in Bar
     */
    static float currentToPressure(float current_mA, float busVoltage_V) {
        if (current_mA <= 0) {
            return INVALID_PRESSURE;
        }

        // Calculate sensor resistance from measured current
        // V = I * (R_series + R_shunt + R_sensor)
        // R_sensor = (V / I) - R_series - R_shunt
        float totalResistance = (busVoltage_V * 1000.0) / current_mA;
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
}

// ========================================
// PUBLIC API
// ========================================

/**
 * Initialize the oil pressure sensor
 *
 * @param address I2C address of the INA226 (default: 0x40)
 * @return true if sensor was initialized successfully
 */
static bool begin(uint8_t address = DEFAULT_I2C_ADDRESS) {
    Internal::i2cAddress = address;

    // Check if INA226 responds
    Wire.beginTransmission(address);
    if (Wire.endTransmission() != 0) {
        Internal::initialized = false;
        Internal::status = STATUS_ERROR;
        return false;
    }

    // Create INA226 instance
    if (Internal::ina226 != nullptr) {
        delete Internal::ina226;
    }
    Internal::ina226 = new INA226_WE(address);

    // Initialize INA226
    if (!Internal::ina226->init()) {
        Internal::initialized = false;
        Internal::status = STATUS_ERROR;
        return false;
    }

    // Configure INA226
    Internal::ina226->setResistorRange(SHUNT_RESISTOR, 0.2);  // 0.1 ohm shunt, 200mA max
    Internal::ina226->setAverage(AVERAGE_16);                  // Hardware averaging
    Internal::ina226->setConversionTime(CONV_TIME_1100);       // 1.1ms conversion
    Internal::ina226->setMeasureMode(CONTINUOUS);              // Continuous measurement

    // Wait for first valid reading
    delay(50);

    // Initialize filter buffer
    for (uint8_t i = 0; i < FILTER_SAMPLES; i++) {
        Internal::pressureBuffer[i] = 0.0;
    }
    Internal::bufferIndex = 0;
    Internal::bufferFilled = false;

    Internal::initialized = true;
    Internal::status = STATUS_OK;
    Internal::lastReadTime = millis();

    return true;
}

/**
 * Read pressure from sensor and update internal values
 * Updates both current pressure and moving average
 *
 * @return Current pressure in Bar, or INVALID_PRESSURE on error
 */
static float readPressure() {
    if (!Internal::initialized || Internal::ina226 == nullptr) {
        Internal::status = STATUS_NOT_INIT;
        return INVALID_PRESSURE;
    }

    // Check for overflow
    if (Internal::ina226->overflow) {
        Internal::status = STATUS_ERROR;
        return INVALID_PRESSURE;
    }

    // Read current
    float current_mA = Internal::ina226->getCurrent_mA();

    // Sanity check
    if (isnan(current_mA) || isinf(current_mA)) {
        Internal::status = STATUS_ERROR;
        return INVALID_PRESSURE;
    }

    // Check for circuit faults
    if (current_mA < CURRENT_MIN_VALID) {
        // Open circuit detected
        Internal::status = STATUS_ERROR;
        return INVALID_PRESSURE;
    }

    if (current_mA > CURRENT_MAX_VALID) {
        // Short circuit detected
        Internal::status = STATUS_ERROR;
        return INVALID_PRESSURE;
    }

    // Read bus voltage for accurate calculation
    float busVoltage_V = Internal::ina226->getBusVoltage_V();

    // Convert to pressure
    float rawPressure = Internal::currentToPressure(current_mA, busVoltage_V);

    if (rawPressure < 0) {
        Internal::status = STATUS_ERROR;
        return INVALID_PRESSURE;
    }

    // Update current pressure
    Internal::currentPressureBar = rawPressure;

    // Apply filter for average
    Internal::averagePressureBar = Internal::applyFilter(rawPressure);

    // Update status based on pressure
    if (Internal::averagePressureBar < PRESSURE_CRITICAL) {
        Internal::status = STATUS_CRITICAL;
    } else if (Internal::averagePressureBar < PRESSURE_WARNING) {
        Internal::status = STATUS_WARNING;
    } else {
        Internal::status = STATUS_OK;
    }

    Internal::lastReadTime = millis();

    return Internal::currentPressureBar;
}

/**
 * Get the current (instantaneous) pressure reading
 *
 * @return Current pressure in Bar
 */
static float getCurrentPressureBar() {
    return Internal::currentPressureBar;
}

/**
 * Get the averaged pressure reading (moving average)
 *
 * @return Average pressure in Bar
 */
static float getAveragePressureBar() {
    return Internal::averagePressureBar;
}

/**
 * Get the averaged pressure reading in PSI
 *
 * @return Average pressure in PSI
 */
static float getAveragePressurePsi() {
    return Internal::averagePressureBar * 14.5038;
}

/**
 * Get the current sensor status
 *
 * @return Status code (STATUS_OK, STATUS_WARNING, STATUS_CRITICAL, STATUS_ERROR, STATUS_NOT_INIT)
 */
static uint8_t getStatus() {
    return Internal::status;
}

/**
 * Check if pressure is below warning threshold
 *
 * @return true if pressure is below warning threshold
 */
static bool isUnderPressure() {
    return Internal::status == STATUS_WARNING || Internal::status == STATUS_CRITICAL;
}

/**
 * Check if sensor has an error
 *
 * @return true if sensor has error or is not initialized
 */
static bool hasError() {
    return Internal::status == STATUS_ERROR || Internal::status == STATUS_NOT_INIT;
}

/**
 * Check if sensor is initialized
 *
 * @return true if sensor is initialized
 */
static bool isInitialized() {
    return Internal::initialized;
}

/**
 * Get time since last successful read
 *
 * @return Milliseconds since last read
 */
static uint32_t getTimeSinceLastRead() {
    if (Internal::lastReadTime == 0) {
        return 0;
    }
    return millis() - Internal::lastReadTime;
}

/**
 * Get pressure as integer value (Bar * 10)
 * Useful for CAN bus packing
 *
 * @return Pressure in Bar * 10 (0-100 for 0-10 Bar)
 */
static uint8_t getAveragePressureX10() {
    if (Internal::status == STATUS_ERROR || Internal::status == STATUS_NOT_INIT) {
        return 0;
    }
    return (uint8_t)constrain((int)(Internal::averagePressureBar * 10.0), 0, 127);
}

/**
 * Reset the moving average filter
 */
static void resetFilter() {
    for (uint8_t i = 0; i < FILTER_SAMPLES; i++) {
        Internal::pressureBuffer[i] = 0.0;
    }
    Internal::bufferIndex = 0;
    Internal::bufferFilled = false;
}

/**
 * Print diagnostic information to Serial
 */
static void printDiagnostics() {
    if (!Internal::initialized || Internal::ina226 == nullptr) {
        Serial.println(F("Oil pressure sensor not initialized"));
        return;
    }

    float current_mA = Internal::ina226->getCurrent_mA();
    float busVoltage_V = Internal::ina226->getBusVoltage_V();
    float shuntVoltage_mV = Internal::ina226->getShuntVoltage_mV();

    Serial.println(F("--- Oil Pressure Diagnostics ---"));
    Serial.print(F("Bus Voltage:    ")); Serial.print(busVoltage_V); Serial.println(F(" V"));
    Serial.print(F("Shunt Voltage:  ")); Serial.print(shuntVoltage_mV); Serial.println(F(" mV"));
    Serial.print(F("Current:        ")); Serial.print(current_mA); Serial.println(F(" mA"));
    Serial.print(F("Pressure:       ")); Serial.print(Internal::averagePressureBar); Serial.println(F(" Bar"));
    Serial.print(F("Pressure:       ")); Serial.print(getAveragePressurePsi()); Serial.println(F(" PSI"));
    Serial.print(F("Status:         "));
    switch (Internal::status) {
        case STATUS_OK: Serial.println(F("OK")); break;
        case STATUS_WARNING: Serial.println(F("WARNING")); break;
        case STATUS_CRITICAL: Serial.println(F("CRITICAL")); break;
        case STATUS_ERROR: Serial.println(F("ERROR")); break;
        case STATUS_NOT_INIT: Serial.println(F("NOT INIT")); break;
    }
    Serial.println(F("--------------------------------"));
}

} // namespace OilPressureSensor

#endif // OIL_PRESSURE_SENSOR_H
