#ifndef SD_LOGGER_H
#define SD_LOGGER_H

#include <Arduino.h>
#include <SD.h>
#include <CanBusInterface.h>
#include "RtcClock.h"

// SD card CSV logger for CAN bus frames.
//
// File layout:
//   /<devId_hex4>/<devId_hex4>_YYYYMMDD.csv
//
// Each row:
//   aircraft_id,timestamp,can_id,len,b0,b1,b2,b3,b4,b5,b6,b7
//
// When the RTC is not set, timestamp is "T+<ms>".  The date portion of the
// filename is only appended when the RTC is set; before that a single file
// named <devId_hex4>_NODATE.csv is used.  When TIME: is received a new dated
// file is created and writing switches to it.
//
// Per-ID throttling: each unique ID is written at most once every THROTTLE_MS.
// IDs that would exceed the limit are silently dropped.

namespace CanMonitor {

static const uint16_t MAX_LOGGED_IDS   = 12;
static const uint32_t THROTTLE_MS      = 500;   // max one write per ID per 500 ms (~2/sec)

class SdLogger {
public:
    SdLogger(uint8_t csPin, RtcClock& rtc, uint16_t deviceId, uint16_t aircraftId)
        : _csPin(csPin), _rtc(rtc), _deviceId(deviceId), _aircraftId(aircraftId),
          _ready(false), _idCount(0) {
        memset(_trackedIds,    0, sizeof(_trackedIds));
        memset(_lastWrittenMs, 0, sizeof(_lastWrittenMs));
        _currentDate[0] = '\0';
    }

    bool begin() {
        if (!SD.begin(_csPin)) return false;

        char folder[8];
        sprintf(folder, "%04X", _deviceId);
        if (!SD.exists(folder)) SD.mkdir(folder);

        _writeStartupLog(folder);
        _ready = true;
        _refreshFilePath();
        return true;
    }

    bool isReady() const { return _ready; }

    // Call after RTC time is set so the logger opens a new dated file.
    void onTimeSet() {
        _refreshFilePath();
    }

    void log(const CanBusInterface::Message& msg) {
        if (!_ready) return;

        uint32_t now = millis();

        // Per-ID throttle
        uint8_t slot = _findOrAddId(msg.id);
        if (slot == 0xFF) return;
        if (now - _lastWrittenMs[slot] < THROTTLE_MS) return;
        _lastWrittenMs[slot] = now;

        // Re-open daily file if date rolled over
        _refreshFilePath();

        File f = SD.open(_filePath, FILE_WRITE);
        if (!f) return;

        char ts[20];
        _rtc.getTimestamp(ts);

        f.print(_aircraftId, HEX);
        f.print(',');
        f.print(ts);
        f.print(',');
        f.print(msg.id, HEX);
        f.print(',');
        f.print(msg.length);
        for (uint8_t i = 0; i < 8; i++) {
            f.print(',');
            if (i < msg.length) {
                if (msg.data[i] < 0x10) f.print('0');
                f.print(msg.data[i], HEX);
            } else {
                f.print(F("00"));
            }
        }
        f.println();
        f.close();
    }

    void setDeviceId(uint16_t id)   { _deviceId   = id; _refreshFilePath(); }
    void setAircraftId(uint16_t id) { _aircraftId = id; }

private:
    uint8_t   _csPin;
    RtcClock& _rtc;
    uint16_t  _deviceId;
    uint16_t  _aircraftId;
    bool      _ready;

    char      _filePath[24];
    char      _currentDate[9];  // "YYYYMMDD" or ""

    uint32_t  _trackedIds[MAX_LOGGED_IDS];
    uint32_t  _lastWrittenMs[MAX_LOGGED_IDS];
    uint8_t   _idCount;

    uint8_t _findOrAddId(uint32_t id) {
        for (uint8_t i = 0; i < _idCount; i++) {
            if (_trackedIds[i] == id) return i;
        }
        if (_idCount >= MAX_LOGGED_IDS) return 0xFF;
        _trackedIds[_idCount]    = id;
        _lastWrittenMs[_idCount] = 0;
        return _idCount++;
    }

    void _refreshFilePath() {
        char date[9];
        _rtc.getDateString(date);

        if (strcmp(date, _currentDate) == 0) return;
        strncpy(_currentDate, date, sizeof(_currentDate));

        char folder[8];
        sprintf(folder, "%04X", _deviceId);
        if (!SD.exists(folder)) SD.mkdir(folder);

        if (_currentDate[0] == '\0') {
            sprintf(_filePath, "%s/%s_NODATE.csv", folder, folder);
        } else {
            sprintf(_filePath, "%s/%s_%s.csv", folder, folder, _currentDate);
        }

        if (!SD.exists(_filePath)) {
            File f = SD.open(_filePath, FILE_WRITE);
            if (f) {
                f.println(F("aircraft_id,timestamp,can_id,len,b0,b1,b2,b3,b4,b5,b6,b7"));
                f.close();
            }
        }
    }

    void _writeStartupLog(const char* folder) {
        char path[20];
        sprintf(path, "%s/startup.log", folder);
        File f = SD.open(path, FILE_WRITE);
        if (!f) return;
        char ts[20];
        _rtc.getTimestamp(ts);
        f.print(ts);
        f.println(F(" EMS CanbusMonitor startup"));
        f.close();
    }
};

} // namespace CanMonitor

#endif // SD_LOGGER_H
