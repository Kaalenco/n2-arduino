#ifndef CAN_BUS_INTERFACE_H
#define CAN_BUS_INTERFACE_H

#include <Arduino.h>

/**
 * Abstract interface for CAN bus communication
 * Provides library-agnostic CAN bus operations
 */
class CanBusInterface {
public:
    enum Speed {
        SPEED_125KBPS,
        SPEED_250KBPS,
        SPEED_500KBPS,
        SPEED_1MBPS
    };

    enum Mode {
        MODE_NORMAL,
        MODE_LOOPBACK,
        MODE_LISTEN_ONLY
    };

    struct Message {
        uint32_t id;
        uint8_t length;
        uint8_t data[8];
        bool extended;
        bool rtr;
    };

    enum Result {
        OK = 0,
        ERROR_INIT_FAILED,
        ERROR_SEND_FAILED,
        ERROR_NO_MESSAGE,
        ERROR_INVALID_PARAM
    };

    virtual ~CanBusInterface() {}
    virtual Result begin(Speed speed, Mode mode = MODE_NORMAL) = 0;
    virtual Result sendMessage(const Message& msg, unsigned long timeout_ms = 1000) = 0;
    virtual bool messageAvailable() = 0;
    virtual Result receiveMessage(Message& msg, unsigned long timeout_ms = 1000) = 0;
    virtual Result setMode(Mode mode) = 0;
    virtual bool isReady() const = 0;
};

#endif // CAN_BUS_INTERFACE_H
