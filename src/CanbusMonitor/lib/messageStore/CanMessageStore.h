#ifndef CAN_MESSAGE_STORE_H
#define CAN_MESSAGE_STORE_H

#include <Arduino.h>
#include <CanBusInterface.h>

// Stores the most recent data frame for each unique CAN ID seen on the bus.
// Capacity is fixed at MAX_TRACKED_IDS; new IDs beyond the limit are silently dropped.
// All state is in RAM — no EEPROM or flash involvement.

namespace CanMonitor {

static const uint8_t MAX_TRACKED_IDS = 12;

struct CanEntry {
    uint32_t id;
    uint8_t  data[8];
    uint8_t  length;
    bool     valid;
};

class CanMessageStore {
public:
    CanMessageStore() : _count(0) {
        for (uint8_t i = 0; i < MAX_TRACKED_IDS; i++) {
            _entries[i].valid = false;
            _entries[i].id    = 0;
        }
    }

    void update(const CanBusInterface::Message& msg) {
        for (uint8_t i = 0; i < _count; i++) {
            if (_entries[i].id == msg.id) {
                _entries[i].length = msg.length;
                memcpy(_entries[i].data, msg.data, msg.length);
                return;
            }
        }
        if (_count < MAX_TRACKED_IDS) {
            _entries[_count].id     = msg.id;
            _entries[_count].length = msg.length;
            _entries[_count].valid  = true;
            memcpy(_entries[_count].data, msg.data, msg.length);
            _count++;
        }
    }

    void clear() {
        _count = 0;
        for (uint8_t i = 0; i < MAX_TRACKED_IDS; i++) {
            _entries[i].valid = false;
        }
    }

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
