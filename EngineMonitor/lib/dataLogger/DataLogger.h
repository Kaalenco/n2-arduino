// DataLogger - Abstract interface for logging sensor data to various outputs
// Provides a common interface for sending sensor data to different destinations:
// - Serial/Terminal (for debugging)
// - LCD Display
// - CAN Bus
// - SD Card
// - etc.

#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

#include <Arduino.h>
#include <TemperatureManager.h>

/**
 * Abstract base class for data logging
 * Concrete implementations handle specific output devices
 */
class DataLogger {
public:
    /**
     * Initialize the logger
     * @return true if successful
     */
    virtual bool begin() = 0;

    /**
     * Log a single sensor reading
     *
     * @param reading Sensor reading to log
     */
    virtual void logReading(const TemperatureManager::SensorReading& reading) = 0;

    /**
     * Log multiple sensor readings
     *
     * @param readings Array of sensor readings
     * @param count Number of readings in array
     */
    virtual void logReadings(const TemperatureManager::SensorReading* readings, uint8_t count) {
        // Default implementation: log each reading individually
        for (uint8_t i = 0; i < count; i++) {
            logReading(readings[i]);
        }
    }

    /**
     * Start a new logging session/frame
     * Called before logging a batch of readings
     */
    virtual void beginFrame() {}

    /**
     * End a logging session/frame
     * Called after logging a batch of readings
     */
    virtual void endFrame() {}

    /**
     * Flush any buffered data
     */
    virtual void flush() {}

    /**
     * Check if logger is ready to accept data
     * @return true if ready
     */
    virtual bool isReady() {
        return true;
    }

    virtual ~DataLogger() {}
};

#endif // DATA_LOGGER_H
