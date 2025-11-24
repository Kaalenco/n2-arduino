// SerialLogger - Logs sensor data to Serial terminal in human-readable format
// Useful for debugging and monitoring during development
//
// Output format (one line per sensor):
// [SensorType] SensorName: XXX°C [Status]
//
// Example:
// [0x18] Ambient: -15°C [OK]
// [0x21] EGT: 425°C [OK]
// [0x30] Oil: 85°C [OK]
// [0x12] CHT1: ERROR

#ifndef SERIAL_LOGGER_H
#define SERIAL_LOGGER_H

#include <Arduino.h>
#include <DataLogger.h>

class SerialLogger : public DataLogger {
private:
    uint32_t baudRate;
    bool initialized;
    bool verbose;  // If true, include sensor type ID and detailed status

public:
    /**
     * Constructor
     *
     * @param baud Baud rate for serial communication (default: 9600)
     * @param verboseMode If true, include sensor IDs and detailed info (default: true)
     */
    SerialLogger(uint32_t baud = 9600, bool verboseMode = true)
        : baudRate(baud), initialized(false), verbose(verboseMode) {}

    /**
     * Initialize the serial logger
     * @return true if successful
     */
    bool begin() override {
        if (!initialized) {
            Serial.begin(baudRate);

            // Wait for serial to be ready (mainly for native USB boards)
            uint32_t startTime = millis();
            while (!Serial && (millis() - startTime) < 2000) {
                ; // Wait up to 2 seconds
            }

            initialized = true;
        }
        return true;
    }

    /**
     * Check if serial is ready
     * @return true if ready
     */
    bool isReady() override {
        return Serial && initialized;
    }

    /**
     * Start a new frame (print header/separator)
     */
    void beginFrame() override {
        if (!isReady()) return;

        Serial.println();
        Serial.println(F("--- Sensor Readings ---"));
    }

    /**
     * Log a single sensor reading
     */
    void logReading(const Common::SensorReading& reading) override {
        if (!isReady()) return;

        // Get sensor name
        const char* sensorName = Common::getSensorName(reading.id);

        if (verbose) {
            // Verbose format with sensor ID
            Serial.print(F("[0x"));
            if (reading.id < 0x10) Serial.print('0');
            Serial.print(reading.id, HEX);
            Serial.print(F("] "));
        }

        // Sensor name (padded to 7 characters for alignment)
        Serial.print(sensorName);
        for (uint8_t i = strlen(sensorName); i < 7; i++) {
            Serial.print(' ');
        }
        Serial.print(F(" Status ["));
        Serial.print(reading.status);
        Serial.print(F("]: "));


        if (reading.success) {
            // Format temperature with sign
            if (reading.value >= 0) {
                Serial.print(' ');  // Extra space for alignment with negative values
            }
            Serial.print(reading.value);
            Serial.print(F("°C"));

            if (verbose) {
                Serial.print(F(" [OK]"));
            }
        } else {
            Serial.print(F("ERROR - No data"));

            if (verbose) {
                Serial.print(F(" [FAIL]"));
            }
        }

        Serial.println();
    }

    /**
     * End frame (print footer/separator)
     */
    void endFrame() override {
        if (!isReady()) return;

        Serial.println();
    }

    /**
     * Flush serial buffer
     */
    void flush() override {
        if (isReady()) {
            Serial.flush();
        }
    }

    /**
     * Set verbose mode
     * @param mode true for verbose output with sensor IDs
     */
    void setVerbose(bool mode) {
        verbose = mode;
    }

    /**
     * Log a formatted message
     * @param message Message to log
     */
    void logMessage(const char* message) {
        if (isReady()) {
            Serial.println(message);
        }
    }

    /**
     * Log a formatted message from flash memory (F() macro)
     * @param message Message to log
     */
    void logMessage(const __FlashStringHelper* message) {
        if (isReady()) {
            Serial.println(message);
        }
    }

    // Print methods that mirror Serial.print behavior (without newline)

    template<typename T>
    void logPrint(T value) {
        if (isReady()) {
            Serial.print(value);
        }
    }

    template<typename T>
    void logPrint(T value, int format) {
        if (isReady()) {
            Serial.print(value, format);
        }
    }

    void logPrint(const __FlashStringHelper* message) {
        if (isReady()) {
            Serial.print(message);
        }
    }

    // Println methods (with newline)

    template<typename T>
    void logPrintln(T value) {
        if (isReady()) {
            Serial.println(value);
        }
    }

    template<typename T>
    void logPrintln(T value, int format) {
        if (isReady()) {
            Serial.println(value, format);
        }
    }

    void logPrintln(const __FlashStringHelper* message) {
        if (isReady()) {
            Serial.println(message);
        }
    }

    void logPrintln() {
        if (isReady()) {
            Serial.println();
        }
    }
};

#endif // SERIAL_LOGGER_H
