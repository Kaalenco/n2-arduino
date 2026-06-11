#ifndef CAN_BUS_INTERFACE_H
#define CAN_BUS_INTERFACE_H

#include <Arduino.h>

/**
 * Abstract interface for CAN bus communication
 * Provides library-agnostic CAN bus operations
 */
class CanBusInterface {
public:
    /**
     * CAN bus speeds
     */
    enum Speed {
        SPEED_125KBPS,
        SPEED_250KBPS,
        SPEED_500KBPS,
        SPEED_1MBPS
    };

    /**
     * CAN bus modes
     */
    enum Mode {
        MODE_NORMAL,
        MODE_LOOPBACK,
        MODE_LISTEN_ONLY
    };

    /**
     * CAN message structure
     */
    struct Message {
        uint32_t id;           // CAN message ID
        uint8_t length;        // Data length (0-8)
        uint8_t data[8];       // Message data
        bool extended;         // Extended ID flag
        bool rtr;              // Remote transmission request flag
    };

    /**
     * Result codes
     */
    enum Result {
        OK = 0,
        ERROR_INIT_FAILED,
        ERROR_SEND_FAILED,
        ERROR_NO_MESSAGE,
        ERROR_INVALID_PARAM
    };

    virtual ~CanBusInterface() {}

    /**
     * Initialize CAN bus hardware
     * @param speed CAN bus speed
     * @param mode Operation mode
     * @return Result code
     */
    virtual Result begin(Speed speed, Mode mode = MODE_NORMAL) = 0;

    /**
     * Send a CAN message
     * @param msg Message to send
     * @param timeout_ms Timeout in milliseconds
     * @return Result code
     */
    virtual Result sendMessage(const Message& msg, unsigned long timeout_ms = 1000) = 0;

    /**
     * Check if a message is available
     * @return true if message available
     */
    virtual bool messageAvailable() = 0;

    /**
     * Receive a CAN message
     * @param msg Reference to store received message
     * @param timeout_ms Timeout in milliseconds
     * @return Result code
     */
    virtual Result receiveMessage(Message& msg, unsigned long timeout_ms = 1000) = 0;

    /**
     * Set operating mode
     * @param mode Desired mode
     * @return Result code
     */
    virtual Result setMode(Mode mode) = 0;

    /**
     * Check if CAN bus is initialized
     * @return true if ready
     */
    virtual bool isReady() const = 0;
};

#endif // CAN_BUS_INTERFACE_H
