// EepromConfigure - CAN bus interface for EEPROM configuration
// Enables reading and modification of EEPROM values via CAN bus messages
//
// Supported message types:
// - CONFIGURE_SET_BYTE: Set a single byte value at an address
// - CONFIGURE_SET_WORD: Set a 2-byte value at an address
// - CONFIGURE_GET_BYTE: Request a single byte value from an address
// - CONFIGURE_GET_WORD: Request a 2-byte value from an address
// - CONFIGURE_BYTE_VALUE: Response with single byte value
// - CONFIGURE_WORD_VALUE: Response with 2-byte value

#ifndef EEPROM_CONFIGURE_H
#define EEPROM_CONFIGURE_H

#include <Arduino.h>
#include <EEPROM.h>
#include <mcp_can.h>

namespace EepromConfig {

/**
 * Message type identifiers for EEPROM configuration via CAN bus
 */
enum ConfigMessageType : uint16_t {
    CONFIGURE_SET_BYTE   = 0x200,  // Set 1-byte value: [addr_lo, addr_hi, value]
    CONFIGURE_SET_WORD   = 0x201,  // Set 2-byte value: [addr_lo, addr_hi, val_lo, val_hi]
    CONFIGURE_GET_BYTE   = 0x202,  // Get 1-byte value: [addr_lo, addr_hi]
    CONFIGURE_GET_WORD   = 0x203,  // Get 2-byte value: [addr_lo, addr_hi]
    CONFIGURE_BYTE_VALUE = 0x210,  // Response 1-byte: [addr_lo, addr_hi, value]
    CONFIGURE_WORD_VALUE = 0x211   // Response 2-byte: [addr_lo, addr_hi, val_lo, val_hi]
};

/**
 * Result codes for configuration operations
 */
enum ConfigResult : uint8_t {
    CONFIG_OK = 0,
    CONFIG_ERROR_NOT_INITIALIZED,
    CONFIG_ERROR_INVALID_ADDRESS,
    CONFIG_ERROR_SEND_FAILED
};

/**
 * EepromConfigure class for CAN bus EEPROM configuration
 *
 * Handles incoming CAN messages to read/write EEPROM values and
 * sends response messages with the current values.
 */
class EepromConfigure {
public:
    /**
     * Constructor
     * @param can Reference to initialized MCP_CAN instance
     */
    EepromConfigure(MCP_CAN& can) : _can(can), _initialized(false) {}

    /**
     * Initialize the EEPROM configurator
     * @return true if initialization successful
     */
    bool begin() {
        _initialized = true;
        return true;
    }

    /**
     * Check if configurator is ready
     * @return true if ready to process messages
     */
    bool isReady() const {
        return _initialized;
    }

    /**
     * Process an incoming CAN message
     * Checks if the message is a configuration message and handles it
     *
     * @param msgId CAN message ID
     * @param len Message data length
     * @param data Message data buffer
     * @return true if message was a configuration message and was processed
     */
    bool processMessage(uint16_t msgId, uint8_t len, uint8_t* data) {
        if (!_initialized) return false;

        switch (msgId) {
            case CONFIGURE_SET_BYTE:
                return handleSetByte(data, len);

            case CONFIGURE_SET_WORD:
                return handleSetWord(data, len);

            case CONFIGURE_GET_BYTE:
                return handleGetByte(data, len);

            case CONFIGURE_GET_WORD:
                return handleGetWord(data, len);

            default:
                return false;  // Not a configuration message
        }
    }

    /**
     * Set a byte value in EEPROM and send confirmation
     *
     * @param address EEPROM address
     * @param value Byte value to write
     * @return Result code
     */
    ConfigResult setByte(uint16_t address, uint8_t value) {
        if (!_initialized) return CONFIG_ERROR_NOT_INITIALIZED;

        // Write to EEPROM
        EEPROM.update(address, value);

        // Send confirmation with the written value
        return sendByteValue(address, value);
    }

    /**
     * Set a word (2-byte) value in EEPROM and send confirmation
     *
     * @param address EEPROM address
     * @param value Word value to write
     * @return Result code
     */
    ConfigResult setWord(uint16_t address, uint16_t value) {
        if (!_initialized) return CONFIG_ERROR_NOT_INITIALIZED;

        // Write to EEPROM (little-endian)
        EEPROM.update(address, value & 0xFF);
        EEPROM.update(address + 1, (value >> 8) & 0xFF);

        // Send confirmation with the written value
        return sendWordValue(address, value);
    }

    /**
     * Get a byte value from EEPROM and send it
     *
     * @param address EEPROM address
     * @return Result code
     */
    ConfigResult getByte(uint16_t address) {
        if (!_initialized) return CONFIG_ERROR_NOT_INITIALIZED;

        uint8_t value = EEPROM.read(address);
        return sendByteValue(address, value);
    }

    /**
     * Get a word (2-byte) value from EEPROM and send it
     *
     * @param address EEPROM address
     * @return Result code
     */
    ConfigResult getWord(uint16_t address) {
        if (!_initialized) return CONFIG_ERROR_NOT_INITIALIZED;

        // Read from EEPROM (little-endian)
        uint16_t value = EEPROM.read(address) | (EEPROM.read(address + 1) << 8);
        return sendWordValue(address, value);
    }

    /**
     * Send a byte value response
     *
     * @param address EEPROM address
     * @param value Byte value
     * @return Result code
     */
    ConfigResult sendByteValue(uint16_t address, uint8_t value) {
        uint8_t data[8] = {0};
        data[0] = address & 0xFF;         // Address low byte
        data[1] = (address >> 8) & 0xFF;  // Address high byte
        data[2] = value;                   // Value

        if (_can.sendMsgBuf(CONFIGURE_BYTE_VALUE, 0, 3, data) == CAN_OK) {
            return CONFIG_OK;
        }
        return CONFIG_ERROR_SEND_FAILED;
    }

    /**
     * Send a word value response
     *
     * @param address EEPROM address
     * @param value Word value
     * @return Result code
     */
    ConfigResult sendWordValue(uint16_t address, uint16_t value) {
        uint8_t data[8] = {0};
        data[0] = address & 0xFF;         // Address low byte
        data[1] = (address >> 8) & 0xFF;  // Address high byte
        data[2] = value & 0xFF;           // Value low byte
        data[3] = (value >> 8) & 0xFF;    // Value high byte

        if (_can.sendMsgBuf(CONFIGURE_WORD_VALUE, 0, 4, data) == CAN_OK) {
            return CONFIG_OK;
        }
        return CONFIG_ERROR_SEND_FAILED;
    }

private:
    MCP_CAN& _can;
    bool _initialized;

    /**
     * Handle CONFIGURE_SET_BYTE message
     * Message format: [addr_lo, addr_hi, value]
     */
    bool handleSetByte(uint8_t* data, uint8_t len) {
        if (len < 3) return false;

        uint16_t address = data[0] | (data[1] << 8);
        uint8_t value = data[2];

        return setByte(address, value) == CONFIG_OK;
    }

    /**
     * Handle CONFIGURE_SET_WORD message
     * Message format: [addr_lo, addr_hi, val_lo, val_hi]
     */
    bool handleSetWord(uint8_t* data, uint8_t len) {
        if (len < 4) return false;

        uint16_t address = data[0] | (data[1] << 8);
        uint16_t value = data[2] | (data[3] << 8);

        return setWord(address, value) == CONFIG_OK;
    }

    /**
     * Handle CONFIGURE_GET_BYTE message
     * Message format: [addr_lo, addr_hi]
     */
    bool handleGetByte(uint8_t* data, uint8_t len) {
        if (len < 2) return false;

        uint16_t address = data[0] | (data[1] << 8);
        return getByte(address) == CONFIG_OK;
    }

    /**
     * Handle CONFIGURE_GET_WORD message
     * Message format: [addr_lo, addr_hi]
     */
    bool handleGetWord(uint8_t* data, uint8_t len) {
        if (len < 2) return false;

        uint16_t address = data[0] | (data[1] << 8);
        return getWord(address) == CONFIG_OK;
    }
};

} // namespace EepromConfig

#endif // EEPROM_CONFIGURE_H
