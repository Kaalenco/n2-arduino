// SensorReading - Base structure for sensor readings
// Provides common fields used across different sensor types
//
// Managers can extend this with sensor-specific fields

#ifndef SENSOR_READING_H
#define SENSOR_READING_H

#include <Arduino.h>
#include <SensorTypes.h>

namespace Common {

/**
 * Base structure to hold sensor reading result
 * Common fields shared by all sensor types
 */
struct SensorReading {
    uint8_t id;           // Sensor ID from SensorTypes.h
    int16_t value;        // Sensor value (interpretation depends on sensor type)
    uint8_t status;       // Status code from sensor
    bool warning;         // True if value exceeds warning threshold
    bool success;         // True if read was successful, false if error
};

/**
 * Get sensor name by sensor ID
 * Provides a generic lookup for all sensor types
 *
 * @param sensorId Sensor ID from SensorTypes.h
 * @return Sensor name string
 */
inline const char* getSensorName(uint8_t sensorId) {
    switch (sensorId) {
        // Temperature sensors
        case SENSOR_TEMPERATURE:       return "Temp";
        case SENSOR_TEMPERATURE_CHT:   return "CHT";
        case SENSOR_TEMPERATURE_CHT_1: return "CHT1";
        case SENSOR_TEMPERATURE_CHT_2: return "CHT2";
        case SENSOR_TEMPERATURE_CHT_3: return "CHT3";
        case SENSOR_TEMPERATURE_CHT_4: return "CHT4";
        case SENSOR_TEMPERATURE_CHT_5: return "CHT5";
        case SENSOR_TEMPERATURE_CHT_6: return "CHT6";
        case SENSOR_TEMPERATURE_AMB:   return "Engine";
        case SENSOR_TEMPERATURE_INTAKE: return "Intake";
        case SENSOR_TEMPERATURE_COCKPIT: return "Cockpit";
        case SENSOR_TEMPERATURE_OIL:   return "OilTemp";
        case SENSOR_TEMPERATURE_WATER: return "Water";
        case SENSOR_TEMPERATURE_EGT:   return "EGT";
        case SENSOR_TEMPERATURE_EGT_1: return "EGT1";
        case SENSOR_TEMPERATURE_EGT_2: return "EGT2";
        case SENSOR_TEMPERATURE_EGT_3: return "EGT3";
        case SENSOR_TEMPERATURE_EGT_4: return "EGT4";

        // Pressure sensors
        case SENSOR_PRESSURE_OIL:      return "OilPres";
        case SENSOR_PRESSURE_FUEL:     return "FuelPrs";
        case SENSOR_PRESSURE_MANIFOLD: return "MAP";

        // Fuel sensors
        case SENSOR_FUEL_LEVEL:        return "Fuel";
        case SENSOR_FUEL_FLOW:         return "Flow";
        case SENSOR_FUEL_PRESSURE:     return "FuelP";

        // Electrical sensors
        case SENSOR_BATTERY_VOLTAGE:   return "Volts";
        case SENSOR_BATTERY_CURRENT:   return "Amps";

        // Navigation sensors
        case SENSOR_ALTIMETER:         return "Alt";
        case SENSOR_MAGNETIC_HEADING:  return "Hdg";
        case SENSOR_GPS_SPEED:         return "GSpd";
        case SENSOR_GPS_ALTITUDE:      return "GAlt";
        case SENSOR_GPS_LATITUDE:      return "Lat";
        case SENSOR_GPS_LONGITUDE:     return "Lon";
        case SENSOR_GPS_SATELLITES:    return "Sats";

        default: return "Unknown";
    }
}

} // namespace Common

#endif // SENSOR_READING_H
