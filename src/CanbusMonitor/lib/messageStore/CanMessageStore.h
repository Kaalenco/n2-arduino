#ifndef CAN_MESSAGE_STORE_H
#define CAN_MESSAGE_STORE_H

#include <Arduino.h>
#include <CanBusInterface.h>

// Stores the most recent data frame for each unique CAN ID seen on the bus.
// Capacity is fixed at MAX_TRACKED_IDS; new IDs beyond the limit are silently dropped.
// All state is in RAM — no EEPROM or flash involvement.
//
// id is uint16_t: all standard CAN IDs are 11-bit (≤ 0x7FF).
// data is capped at 4 bytes: sufficient for display (all known IDs use ≤ 2 bytes;
// unknown IDs are shown as 4 hex bytes).

namespace CanMonitor {

static const uint8_t MAX_TRACKED_IDS  = 8;
static const uint8_t ENTRY_DATA_BYTES = 4;

struct CanEntry {
    uint16_t id;
    uint8_t  data[ENTRY_DATA_BYTES];
    uint8_t  length;  // capped at ENTRY_DATA_BYTES
};

class CanMessageStore {
public:
    CanMessageStore() : _count(0) {}

    void update(const CanBusInterface::Message& msg) {
        uint16_t id16 = (uint16_t)msg.id;
        for (uint8_t i = 0; i < _count; i++) {
            if (_entries[i].id == id16) {
                uint8_t len = (msg.length < ENTRY_DATA_BYTES) ? msg.length : ENTRY_DATA_BYTES;
                _entries[i].length = len;
                memcpy(_entries[i].data, msg.data, len);
                return;
            }
        }
        if (_count < MAX_TRACKED_IDS) {
            uint8_t len = (msg.length < ENTRY_DATA_BYTES) ? msg.length : ENTRY_DATA_BYTES;
            _entries[_count].id     = id16;
            _entries[_count].length = len;
            memcpy(_entries[_count].data, msg.data, len);
            _count++;
        }
    }

    void clear() { _count = 0; }

    uint8_t count() const { return _count; }

    const CanEntry* getAt(uint8_t index) const {
        if (index >= _count) return nullptr;
        return &_entries[index];
    }

private:
    CanEntry _entries[MAX_TRACKED_IDS];
    uint8_t  _count;
};

} // namespace CanMonitor

#endif // CAN_MESSAGE_STORE_H
