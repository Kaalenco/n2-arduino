#ifndef CAN_BUS_MCP2515_H
#define CAN_BUS_MCP2515_H

#include "CanBusInterface.h"
#include <MCP2515.h>
#include <SPI.h>

/**
 * MCP2515 CAN bus implementation using Watterott library
 * Wraps the static MCP2515 library methods
 */
class CanBusMCP2515 : public CanBusInterface {
public:
    /**
     * Constructor
     */
    CanBusMCP2515() : _initialized(false) {}

    /**
     * Initialize CAN bus hardware
     * @param speed CAN bus speed
     * @param mode Operation mode
     * @return Result code
     */
    Result begin(Speed speed, Mode mode = MODE_NORMAL) override {
        // Map abstract speed to MCP2515 baud constant
        int baudConst;
        switch (speed) {
            case SPEED_125KBPS: baudConst = CAN_BAUD_125K; break;
            case SPEED_250KBPS: baudConst = CAN_BAUD_250K; break;
            case SPEED_500KBPS: baudConst = CAN_BAUD_500K; break;
            default: return ERROR_INVALID_PARAM;
        }

        // Initialize CAN controller
        if (!MCP2515::initCAN(baudConst)) {
            return ERROR_INIT_FAILED;
        }

        // Set operating mode
        Result modeResult = setMode(mode);
        if (modeResult != OK) {
            return modeResult;
        }

        _initialized = true;
        return OK;
    }

    /**
     * Send a CAN message
     * @param msg Message to send
     * @param timeout_ms Timeout in milliseconds
     * @return Result code
     */
    Result sendMessage(const Message& msg, unsigned long timeout_ms = 1000) override {
        if (!_initialized) return ERROR_INIT_FAILED;

        // Convert to CANMSG format
        CANMSG canMsg;
        canMsg.adrsValue = msg.id;
        canMsg.isExtendedAdrs = msg.extended;
        canMsg.rtr = msg.rtr;
        canMsg.dataLength = msg.length;
        memcpy(canMsg.data, msg.data, msg.length);

        boolean result = MCP2515::transmitCANMessage(canMsg, timeout_ms);
        return result ? OK : ERROR_SEND_FAILED;
    }

    /**
     * Check if a message is available
     * @return true if message available
     */
    bool messageAvailable() override {
        if (!_initialized) return false;

        // Read CANINTF register to check RX0IF flag without consuming message
        uint8_t flags = readReg(CANINTF);
        return (flags & (1 << RX0IF)) != 0;
    }

    /**
     * Receive a CAN message
     * @param msg Reference to store received message
     * @param timeout_ms Timeout in milliseconds
     * @return Result code
     */
    Result receiveMessage(Message& msg, unsigned long timeout_ms = 1000) override {
        if (!_initialized) return ERROR_INIT_FAILED;

        CANMSG canMsg;
        boolean result = MCP2515::receiveCANMessage(&canMsg, timeout_ms);

        if (!result) {
            return ERROR_NO_MESSAGE;
        }

        // Convert from CANMSG format
        msg.id = canMsg.adrsValue;
        msg.extended = canMsg.isExtendedAdrs;
        msg.rtr = canMsg.rtr;
        msg.length = canMsg.dataLength;
        memcpy(msg.data, canMsg.data, canMsg.dataLength);

        return OK;
    }

    /**
     * Set operating mode
     * @param mode Desired mode
     * @return Result code
     */
    Result setMode(Mode mode) override {
        boolean result;

        switch (mode) {
            case MODE_NORMAL:
                result = MCP2515::setCANNormalMode(false);
                break;
            case MODE_LISTEN_ONLY:
                result = MCP2515::setCANReceiveonlyMode();
                break;
            case MODE_LOOPBACK:
                result = setLoopbackMode();
                break;
            default:
                return ERROR_INVALID_PARAM;
        }

        return result ? OK : ERROR_INIT_FAILED;
    }

    /**
     * Check if CAN bus is initialized
     * @return true if ready
     */
    bool isReady() const override {
        return _initialized;
    }

    /**
     * Clear any pending messages from RX buffer
     * Useful for clearing stale messages after initialization
     */
    void clearRxBuffer() {
        if (!_initialized) return;

        // Read and discard any pending messages with short timeout
        CANMSG dummy;
        while (MCP2515::receiveCANMessage(&dummy, 10)) {
            // Keep clearing until no more messages
        }
    }

private:
    bool _initialized;

    // MCP2515 register addresses
    static const uint8_t CANSTAT = 0x0E;
    static const uint8_t CANCTRL = 0x0F;
    static const uint8_t CANINTF = 0x2C;
    static const uint8_t SLAVESELECT = 10;

    // CANINTF bits
    static const uint8_t RX0IF = 0;

    // SPI commands
    static const uint8_t WRITE = 0x02;
    static const uint8_t READ = 0x03;

    /**
     * Set loopback mode (not available in base library)
     * Loopback mode: REQOP<2:0> = 010
     * @return true if successful
     */
    bool setLoopbackMode() {
        // REQOP2<2:0> = 010 for loopback mode
        // ABAT = 0, do not abort pending transmission
        // OSM = 0, not one shot
        // CLKEN = 1, disable output clock
        // CLKPRE = 0b11, clk/8
        uint8_t settings = 0b01000111;

        writeReg(CANCTRL, settings);

        // Read mode and make sure it is loopback (mode bits = 010 = 2)
        uint8_t mode = readReg(CANSTAT) >> 5;
        return (mode == 2);
    }

    /**
     * Write to MCP2515 register
     */
    void writeReg(uint8_t regno, uint8_t val) {
        digitalWrite(SLAVESELECT, LOW);
        SPI.transfer(WRITE);
        SPI.transfer(regno);
        SPI.transfer(val);
        digitalWrite(SLAVESELECT, HIGH);
    }

    /**
     * Read from MCP2515 register
     */
    uint8_t readReg(uint8_t regno) {
        digitalWrite(SLAVESELECT, LOW);
        SPI.transfer(READ);
        SPI.transfer(regno);
        uint8_t val = SPI.transfer(0);
        digitalWrite(SLAVESELECT, HIGH);
        return val;
    }
};

#endif // CAN_BUS_MCP2515_H
