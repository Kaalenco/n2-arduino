#ifndef TACH_LOGGER_H
#define TACH_LOGGER_H

#include <CanBusInterface.h>
#include "TachDataPacker.h"

namespace TachLogging {

class TachLogger {
public:
    explicit TachLogger(CanBusInterface& can) : _can(can) {}

    bool begin(CanBusInterface::Speed speed) {
        return _can.begin(speed) == CanBusInterface::OK;
    }

    CanBusInterface& getCan() { return _can; }
    bool isReady() const { return _can.isReady(); }

    bool send(uint16_t canId, uint16_t rpm, bool overSpeed) {
        TachData data { rpm, overSpeed };
        uint8_t buffer[8];
        TachDataPacker::pack(data, buffer);

        CanBusInterface::Message msg;
        msg.id       = canId;
        msg.length   = 4;
        msg.extended = false;
        msg.rtr      = false;
        memcpy(msg.data, buffer, 8);

        return _can.sendMessage(msg) == CanBusInterface::OK;
    }

private:
    CanBusInterface& _can;
};

} // namespace TachLogging

#endif // TACH_LOGGER_H
