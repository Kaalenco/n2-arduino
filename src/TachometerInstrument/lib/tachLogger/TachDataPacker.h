#ifndef TACH_DATA_PACKER_H
#define TACH_DATA_PACKER_H

#include <Arduino.h>

// CAN frame layout (8 bytes, 4 used):
//   Byte 0-1: RPM (uint16_t, little-endian, 0-9999)
//   Byte 2:   flags (see TachFlags)
//   Bytes 3-7: reserved (0)

namespace TachLogging {

enum TachFlags : uint8_t {
    TACH_FLAG_NONE       = 0x00,
    TACH_FLAG_OVER_SPEED = 0x01,
    TACH_FLAG_ACTIVE     = 0x02,
};

struct TachData {
    uint16_t rpm;
    bool overSpeed;
};

class TachDataPacker {
public:
    static void pack(const TachData& data, uint8_t* buffer) {
        buffer[0] = data.rpm & 0xFF;
        buffer[1] = (data.rpm >> 8) & 0xFF;
        buffer[2] = TACH_FLAG_ACTIVE | (data.overSpeed ? TACH_FLAG_OVER_SPEED : TACH_FLAG_NONE);
        buffer[3] = 0;
        buffer[4] = 0;
        buffer[5] = 0;
        buffer[6] = 0;
        buffer[7] = 0;
    }

    static void unpack(const uint8_t* buffer, TachData& data) {
        data.rpm = buffer[0] | ((uint16_t)buffer[1] << 8);
        data.overSpeed = (buffer[2] & TACH_FLAG_OVER_SPEED) != 0;
    }
};

} // namespace TachLogging

#endif // TACH_DATA_PACKER_H
