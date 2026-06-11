#ifndef EEPROM_CONFIGURE_H
#define EEPROM_CONFIGURE_H

#include <Arduino.h>
#include <EEPROM.h>
#include "CanBusInterface.h"

// CAN bus interface for reading and writing EEPROM values remotely.
// Handles CONFIGURE_SET/GET messages and sends VALUE responses.
// Message IDs are shared with the EngineMonitor so a single host tool
// can configure any instrument on the bus.

namespace EepromConfig {

enum ConfigMessageType : uint16_t {
    CONFIGURE_SET_BYTE   = 0x200,
    CONFIGURE_SET_WORD   = 0x201,
    CONFIGURE_GET_BYTE   = 0x202,
    CONFIGURE_GET_WORD   = 0x203,
    CONFIGURE_BYTE_VALUE = 0x210,
    CONFIGURE_WORD_VALUE = 0x211,
};

enum ConfigResult : uint8_t {
    CONFIG_OK = 0,
    CONFIG_ERROR_NOT_INITIALIZED,
    CONFIG_ERROR_INVALID_ADDRESS,
    CONFIG_ERROR_SEND_FAILED,
};

class EepromConfigure {
public:
    explicit EepromConfigure(CanBusInterface& can) : _can(can), _initialized(false) {}

    bool begin() { _initialized = true; return true; }
    bool isReady() const { return _initialized; }

    bool processMessage(const CanBusInterface::Message& msg) {
        return processMessage(msg.id, msg.length, (uint8_t*)msg.data);
    }

    bool processMessage(uint16_t msgId, uint8_t len, uint8_t* data) {
        if (!_initialized) return false;
        switch (msgId) {
            case CONFIGURE_SET_BYTE: return handleSetByte(data, len);
            case CONFIGURE_SET_WORD: return handleSetWord(data, len);
            case CONFIGURE_GET_BYTE: return handleGetByte(data, len);
            case CONFIGURE_GET_WORD: return handleGetWord(data, len);
            default: return false;
        }
    }

    ConfigResult setByte(uint16_t address, uint8_t value) {
        if (!_initialized) return CONFIG_ERROR_NOT_INITIALIZED;
        EEPROM.update(address, value);
        return sendByteValue(address, value);
    }

    ConfigResult setWord(uint16_t address, uint16_t value) {
        if (!_initialized) return CONFIG_ERROR_NOT_INITIALIZED;
        EEPROM.update(address,     value & 0xFF);
        EEPROM.update(address + 1, (value >> 8) & 0xFF);
        return sendWordValue(address, value);
    }

    ConfigResult getByte(uint16_t address) {
        if (!_initialized) return CONFIG_ERROR_NOT_INITIALIZED;
        return sendByteValue(address, EEPROM.read(address));
    }

    ConfigResult getWord(uint16_t address) {
        if (!_initialized) return CONFIG_ERROR_NOT_INITIALIZED;
        uint16_t value = EEPROM.read(address) | (EEPROM.read(address + 1) << 8);
        return sendWordValue(address, value);
    }

    ConfigResult sendByteValue(uint16_t address, uint8_t value) {
        CanBusInterface::Message msg;
        msg.id = CONFIGURE_BYTE_VALUE; msg.length = 3;
        msg.extended = false; msg.rtr = false;
        msg.data[0] = address & 0xFF;
        msg.data[1] = (address >> 8) & 0xFF;
        msg.data[2] = value;
        return _can.sendMessage(msg) == CanBusInterface::OK ? CONFIG_OK : CONFIG_ERROR_SEND_FAILED;
    }

    ConfigResult sendWordValue(uint16_t address, uint16_t value) {
        CanBusInterface::Message msg;
        msg.id = CONFIGURE_WORD_VALUE; msg.length = 4;
        msg.extended = false; msg.rtr = false;
        msg.data[0] = address & 0xFF;
        msg.data[1] = (address >> 8) & 0xFF;
        msg.data[2] = value & 0xFF;
        msg.data[3] = (value >> 8) & 0xFF;
        return _can.sendMessage(msg) == CanBusInterface::OK ? CONFIG_OK : CONFIG_ERROR_SEND_FAILED;
    }

private:
    CanBusInterface& _can;
    bool _initialized;

    bool handleSetByte(uint8_t* data, uint8_t len) {
        if (len < 3) return false;
        return setByte(data[0] | (data[1] << 8), data[2]) == CONFIG_OK;
    }
    bool handleSetWord(uint8_t* data, uint8_t len) {
        if (len < 4) return false;
        return setWord(data[0] | (data[1] << 8), data[2] | (data[3] << 8)) == CONFIG_OK;
    }
    bool handleGetByte(uint8_t* data, uint8_t len) {
        if (len < 2) return false;
        return getByte(data[0] | (data[1] << 8)) == CONFIG_OK;
    }
    bool handleGetWord(uint8_t* data, uint8_t len) {
        if (len < 2) return false;
        return getWord(data[0] | (data[1] << 8)) == CONFIG_OK;
    }
};

} // namespace EepromConfig

#endif // EEPROM_CONFIGURE_H
