#ifndef CANBUS_RECEIVER_H
#define CANBUS_RECEIVER_H

#include <Arduino.h>
#include <mcp_can.h>
#include <SensorTypes.h>
#include <SensorReading.h>
#include "EngineData1Decoder.h"

namespace CanbusReceiver {

/**
 * Message type identifiers for CAN bus communication
 * Must match the sender's message types
 */
enum MessageType : uint16_t {
    ENGINE_DATA_1 = 0x100,    // EGT1, CHT1, OIL_TEMP, ENGINE_AMBIENT, OIL_PRESSURE, ALERTS
    ENGINE_DATA_2 = 0x101,    // Reserved for future use
    FUEL_DATA = 0x102,        // Reserved for future use
    ELECTRICAL_DATA = 0x103   // Reserved for future use
};

/**
 * Number of sensor readings from ENGINE_DATA_1 message
 */
static const uint8_t ENGINE_DATA_1_SENSOR_COUNT = 5;

/**
 * CAN bus receiver for decoding sensor data
 *
 * Receives CAN messages from the EngineMonitor and decodes them
 * into SensorReading structures for display or further processing.
 */
class CanbusReceiver {
public:
    /**
     * Constructor
     * @param csPin Chip select pin for MCP2515
     */
    CanbusReceiver(uint8_t csPin) : _can(csPin), _csPin(csPin), _initialized(false) {}

    /**
     * Initialize the CAN bus interface
     * @param canSpeed CAN bus speed (e.g., CAN_500KBPS)
     * @param clockSpeed MCP2515 clock speed (e.g., MCP_8MHZ)
     * @return true if initialization successful
     */
    bool begin(uint8_t canSpeed = CAN_500KBPS, uint8_t clockSpeed = MCP_8MHZ) {
        if (_can.begin(MCP_ANY, canSpeed, clockSpeed) == CAN_OK) {
            _can.setMode(MCP_NORMAL);
            _initialized = true;
            return true;
        }
        _initialized = false;
        return false;
    }

    /**
     * Check if CAN bus is initialized and ready
     * @return true if ready to receive messages
     */
    bool isReady() const {
        return _initialized;
    }

    /**
     * Check if a message is available
     * @return true if a message is waiting to be read
     */
    bool messageAvailable() {
        if (!_initialized) return false;
        return _can.checkReceive() == CAN_MSGAVAIL;
    }

    /**
     * Receive and decode the next available message
     *
     * @param readings Array to store decoded sensor readings
     * @param maxReadings Size of the readings array
     * @param count Output: number of readings decoded
     * @return Message type ID, or 0 if no message or error
     */
    uint16_t receiveMessage(Common::SensorReading* readings, uint8_t maxReadings, uint8_t& count) {
        count = 0;
        if (!_initialized) return 0;

        unsigned long msgId;
        uint8_t len;
        uint8_t buffer[8];

        if (_can.readMsgBuf(&msgId, &len, buffer) != CAN_OK) {
            return 0;
        }

        // Decode based on message type
        switch (msgId) {
            case ENGINE_DATA_1:
                count = decodeEngineData1(buffer, readings, maxReadings);
                return ENGINE_DATA_1;

            // Future message types can be added here
            case ENGINE_DATA_2:
            case FUEL_DATA:
            case ELECTRICAL_DATA:
                // Not yet implemented
                return (uint16_t)msgId;

            default:
                // Unknown message type
                return (uint16_t)msgId;
        }
    }

    /**
     * Get raw ENGINE_DATA_1 structure from buffer
     * Use this if you need access to all fields including alerts
     *
     * @param buffer 8-byte CAN message buffer
     * @param data Output data structure
     */
    static void decodeEngineData1Raw(const uint8_t* buffer, EngineData1& data) {
        EngineData1Decoder::unpack(buffer, data);
    }

    /**
     * Get the last received alerts from ENGINE_DATA_1
     * @return Alert flags
     */
    uint16_t getLastAlerts() const {
        return _lastAlerts;
    }

private:
    MCP_CAN _can;
    uint8_t _csPin;
    bool _initialized;
    uint16_t _lastAlerts;

    /**
     * Decode ENGINE_DATA_1 message into sensor readings
     *
     * @param buffer 8-byte CAN message buffer
     * @param readings Array to store decoded readings
     * @param maxReadings Size of the readings array
     * @return Number of readings decoded
     */
    uint8_t decodeEngineData1(const uint8_t* buffer, Common::SensorReading* readings, uint8_t maxReadings) {
        if (maxReadings < ENGINE_DATA_1_SENSOR_COUNT) {
            return 0;  // Not enough space
        }

        EngineData1 data;
        EngineData1Decoder::unpack(buffer, data);

        // Store alerts for later access
        _lastAlerts = data.alerts;

        uint8_t idx = 0;

        // EGT1
        readings[idx].id = SENSOR_TEMPERATURE_EGT_1;
        readings[idx].value = data.egt1Celsius;
        readings[idx].status = 0;
        readings[idx].warning = data.egt1OverTemp;
        readings[idx].success = true;
        idx++;

        // CHT1
        readings[idx].id = SENSOR_TEMPERATURE_CHT_1;
        readings[idx].value = data.cht1Celsius;
        readings[idx].status = 0;
        readings[idx].warning = data.cht1OverTemp;
        readings[idx].success = true;
        idx++;

        // Oil Temperature
        readings[idx].id = SENSOR_TEMPERATURE_OIL;
        readings[idx].value = data.oilTempCelsius;
        readings[idx].status = 0;
        readings[idx].warning = data.oilTempOverTemp;
        readings[idx].success = true;
        idx++;

        // Engine Ambient Temperature
        readings[idx].id = SENSOR_TEMPERATURE_AMB;
        readings[idx].value = data.engineAmbientCelsius;
        readings[idx].status = 0;
        readings[idx].warning = data.engineAmbientOverTemp;
        readings[idx].success = true;
        idx++;

        // Oil Pressure (value is in bar * 10)
        readings[idx].id = SENSOR_PRESSURE_OIL;
        readings[idx].value = data.oilPressureX10;
        readings[idx].status = 0;
        readings[idx].warning = data.oilPressureUnder;
        readings[idx].success = true;
        idx++;

        return idx;
    }
};

} // namespace CanbusReceiver

#endif // CANBUS_RECEIVER_H
