#ifndef CAN_BUS_MCP2515_H
#define CAN_BUS_MCP2515_H

#include "CanBusInterface.h"
#include <MCP2515.h>
#include <SPI.h>

class CanBusMCP2515 : public CanBusInterface {
public:
    CanBusMCP2515() : _initialized(false) {}

    Result begin(Speed speed, Mode mode = MODE_NORMAL) override {
        int baudConst;
        switch (speed) {
            case SPEED_125KBPS: baudConst = CAN_BAUD_125K; break;
            case SPEED_250KBPS: baudConst = CAN_BAUD_250K; break;
            case SPEED_500KBPS: baudConst = CAN_BAUD_500K; break;
            default: return ERROR_INVALID_PARAM;
        }

        if (!MCP2515::initCAN(baudConst)) return ERROR_INIT_FAILED;

        Result modeResult = setMode(mode);
        if (modeResult != OK) return modeResult;

        _initialized = true;
        return OK;
    }

    Result sendMessage(const Message& msg, unsigned long timeout_ms = 1000) override {
        if (!_initialized) return ERROR_INIT_FAILED;

        CANMSG canMsg;
        canMsg.adrsValue      = msg.id;
        canMsg.isExtendedAdrs = msg.extended;
        canMsg.rtr            = msg.rtr;
        canMsg.dataLength     = msg.length;
        memcpy(canMsg.data, msg.data, msg.length);

        return MCP2515::transmitCANMessage(canMsg, timeout_ms) ? OK : ERROR_SEND_FAILED;
    }

    bool messageAvailable() override {
        if (!_initialized) return false;
        return (readReg(CANINTF) & (1 << RX0IF)) != 0;
    }

    Result receiveMessage(Message& msg, unsigned long timeout_ms = 1000) override {
        if (!_initialized) return ERROR_INIT_FAILED;

        CANMSG canMsg;
        if (!MCP2515::receiveCANMessage(&canMsg, timeout_ms)) return ERROR_NO_MESSAGE;

        msg.id       = canMsg.adrsValue;
        msg.extended = canMsg.isExtendedAdrs;
        msg.rtr      = canMsg.rtr;
        msg.length   = canMsg.dataLength;
        memcpy(msg.data, canMsg.data, canMsg.dataLength);
        return OK;
    }

    Result setMode(Mode mode) override {
        boolean result;
        switch (mode) {
            case MODE_NORMAL:      result = MCP2515::setCANNormalMode(false);   break;
            case MODE_LISTEN_ONLY: result = MCP2515::setCANReceiveonlyMode();   break;
            case MODE_LOOPBACK:    result = setLoopbackMode();                  break;
            default: return ERROR_INVALID_PARAM;
        }
        return result ? OK : ERROR_INIT_FAILED;
    }

    bool isReady() const override { return _initialized; }

private:
    bool _initialized;

    static const uint8_t CANSTAT    = 0x0E;
    static const uint8_t CANCTRL    = 0x0F;
    static const uint8_t CANINTF    = 0x2C;
    static const uint8_t SLAVESELECT = 10;
    static const uint8_t RX0IF      = 0;
    static const uint8_t WRITE_CMD  = 0x02;
    static const uint8_t READ_CMD   = 0x03;

    bool setLoopbackMode() {
        writeReg(CANCTRL, 0b01000111);
        return (readReg(CANSTAT) >> 5) == 2;
    }

    void writeReg(uint8_t regno, uint8_t val) {
        digitalWrite(SLAVESELECT, LOW);
        SPI.transfer(WRITE_CMD);
        SPI.transfer(regno);
        SPI.transfer(val);
        digitalWrite(SLAVESELECT, HIGH);
    }

    uint8_t readReg(uint8_t regno) {
        digitalWrite(SLAVESELECT, LOW);
        SPI.transfer(READ_CMD);
        SPI.transfer(regno);
        uint8_t val = SPI.transfer(0);
        digitalWrite(SLAVESELECT, HIGH);
        return val;
    }
};

#endif // CAN_BUS_MCP2515_H
