#ifndef CANBUS_LOGGER_H
#define CANBUS_LOGGER_H

#include <Arduino.h>
#include <mcp_can.h>

namespace CanbusLogging {

/**
 * Message type identifiers for CAN bus communication
 */
enum MessageType : uint16_t {
    ENGINE_DATA_1 = 0x100,    // EGT1, CHT1, OIL_TEMP, ENGINE_AMBIENT, OIL_PRESSURE, ALERTS
    ENGINE_DATA_2 = 0x101,    // Reserved for future use
    FUEL_DATA = 0x102,        // Reserved for future use
    ELECTRICAL_DATA = 0x103   // Reserved for future use
};

/**
 * Abstract base class for CAN bus data loggers
 *
 * Provides common CAN bus initialization and communication functionality.
 * Subclasses implement specific message types and data packing.
 */
class CanbusLogger {
public:
    /**
     * Constructor
     * @param csPin Chip select pin for MCP2515
     */
    CanbusLogger(uint8_t csPin) : _can(csPin), _csPin(csPin), _initialized(false) {}

    /**
     * Initialize the CAN bus interface
     * @param canSpeed CAN bus speed (e.g., CAN_500KBPS)
     * @param clockSpeed MCP2515 clock speed (e.g., MCP_8MHZ)
     * @return true if initialization successful
     */
    virtual bool begin(uint8_t canSpeed = CAN_500KBPS, uint8_t clockSpeed = MCP_8MHZ) {
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
     * @return true if ready to send messages
     */
    virtual bool isReady() const {
        return _initialized;
    }

    /**
     * Get reference to the underlying MCP_CAN instance
     * Allows sharing the CAN bus with other components
     * @return Reference to MCP_CAN instance
     */
    MCP_CAN& getCan() {
        return _can;
    }

    /**
     * Get the message type identifier for this logger
     * @return CAN message ID
     */
    virtual uint16_t getMessageType() const = 0;

    /**
     * Send the current data frame
     * Must be implemented by subclasses to pack and send their specific data
     * @return true if message sent successfully
     */
    virtual bool sendFrame() = 0;

    virtual ~CanbusLogger() {}

protected:
    MCP_CAN _can;
    uint8_t _csPin;
    bool _initialized;

    /**
     * Send a CAN message
     * @param msgId Message identifier
     * @param data Pointer to 8-byte data buffer
     * @return true if sent successfully
     */
    bool sendMessage(uint16_t msgId, uint8_t* data) {
        if (!_initialized) return false;
        return _can.sendMsgBuf(msgId, 0, 8, data) == CAN_OK;
    }

    /**
     * Pack a temperature value with warning bit into 11 bits
     * Format: 10 bits for value (0-1023), 1 bit for over-temp warning
     *
     * @param celsius Temperature in degrees Celsius (0-1023)
     * @param overTemp Over-temperature warning flag
     * @return Packed 11-bit value
     */
    static uint16_t packTemperature(int16_t celsius, bool overTemp) {
        uint16_t value = constrain(celsius, 0, 1023);
        if (overTemp) {
            value |= 0x0400;  // Set bit 10 (over-temp warning)
        }
        return value;
    }

    /**
     * Pack an oil pressure value with warning bit into 8 bits
     * Format: 7 bits for value (0-127, representing 0-12.7 bar), 1 bit for under-pressure
     *
     * @param pressureX10 Pressure in bar * 10 (0-120)
     * @param underPressure Under-pressure warning flag
     * @return Packed 8-bit value
     */
    static uint8_t packOilPressure(uint8_t pressureX10, bool underPressure) {
        uint8_t value = constrain(pressureX10, 0, 127);
        if (underPressure) {
            value |= 0x80;  // Set bit 7 (under-pressure warning)
        }
        return value;
    }
};

} // namespace CanbusLogging

#endif // CANBUS_LOGGER_H
