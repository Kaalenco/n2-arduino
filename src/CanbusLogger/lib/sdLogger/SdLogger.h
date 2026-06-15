#ifndef SD_LOGGER_H
#define SD_LOGGER_H

#include <Arduino.h>
#include <SD.h>
#include <CanBusInterface.h>
#include "RtcClock.h"

// Dedicated CAN bus SD logger.
//
// File layout:
//   /<devId_hex4>/<devId_hex4>_YYYYMMDD.csv
//
// Each row:
//   aircraft_id,timestamp,can_id,len,b0,b1,b2,b3,b4,b5,b6,b7
//
// The log file is kept open and flushed to SD every FLUSH_INTERVAL_MS to
// minimise write latency while limiting data loss on power loss.
// File rotation happens when the calendar date changes.
//
// All CAN frames are logged (no per-ID throttle).

namespace CanLogger {

static const uint32_t FLUSH_INTERVAL_MS  = 5000;   // flush every 5 s
static const uint32_t ROTATE_CHECK_MS    = 10000;  // check date rotation every 10 s

class SdLogger {
public:
    SdLogger(uint8_t csPin, RtcClock& rtc, uint16_t deviceId, uint16_t aircraftId)
        : _csPin(csPin), _rtc(rtc), _deviceId(deviceId), _aircraftId(aircraftId),
          _ready(false), _logging(false),
          _lastFlushMs(0), _lastRotateMs(0),
          _framesLogged(0), _framesDropped(0) {
        _filePath[0]    = '\0';
        _currentDate[0] = '\0';
    }

    bool begin() {
        if (!SD.begin(_csPin)) return false;

        char folder[8];
        sprintf(folder, "%04X", _deviceId);
        if (!SD.exists(folder)) SD.mkdir(folder);

        _writeStartupLog(folder);
        _ready = true;
        _openLogFile();
        return true;
    }

    bool isReady()   const { return _ready;   }
    bool isLogging() const { return _logging;  }

    void log(const CanBusInterface::Message& msg) {
        if (!_ready || !_logging) { _framesDropped++; return; }

        char ts[20];
        _rtc.getTimestamp(ts);

        _logFile.print(_aircraftId, HEX);
        _logFile.print(',');
        _logFile.print(ts);
        _logFile.print(',');
        _logFile.print(msg.id, HEX);
        _logFile.print(',');
        _logFile.print(msg.length);
        for (uint8_t i = 0; i < 8; i++) {
            _logFile.print(',');
            if (i < msg.length) {
                if (msg.data[i] < 0x10) _logFile.print('0');
                _logFile.print(msg.data[i], HEX);
            } else {
                _logFile.print(F("00"));
            }
        }
        _logFile.println();
        _framesLogged++;
    }

    // Call from loop() to handle periodic flush and date-based file rotation.
    void tick() {
        if (!_ready) return;
        unsigned long now = millis();

        if (_logging && (now - _lastFlushMs >= FLUSH_INTERVAL_MS)) {
            _logFile.flush();
            _lastFlushMs = now;
        }

        if (now - _lastRotateMs >= ROTATE_CHECK_MS) {
            _lastRotateMs = now;
            if (_logging) _checkRotation();
        }
    }

    // Call after RTC time is set so the logger opens a new dated file.
    void onTimeSet() {
        if (!_ready) return;
        _closeLogFile();
        _currentDate[0] = '\0';
        _openLogFile();
    }

    // Pause/resume for SD operations that need exclusive access (LISTFILES, DOWNLOAD).
    void pause() {
        if (_logging) {
            _logFile.flush();
            _logFile.close();
            _logging = false;
        }
    }

    void resume() {
        if (_ready && !_logging) _openLogFile();
    }

    void setDeviceId(uint16_t id) {
        _deviceId = id;
        if (_ready) {
            pause();
            char folder[8];
            sprintf(folder, "%04X", _deviceId);
            if (!SD.exists(folder)) SD.mkdir(folder);
            _currentDate[0] = '\0';
            resume();
        }
    }

    void setAircraftId(uint16_t id) { _aircraftId = id; }

    uint32_t framesLogged()  const { return _framesLogged;  }
    uint32_t framesDropped() const { return _framesDropped; }

    // List all files in the device folder to Serial.
    void listFiles() {
        bool wasLogging = _logging;
        if (wasLogging) pause();

        char folder[8];
        sprintf(folder, "%04X", _deviceId);
        File dir = SD.open(folder);
        if (!dir) {
            Serial.println(F("ERR: folder not found"));
        } else {
            while (true) {
                File entry = dir.openNextFile();
                if (!entry) break;
                if (!entry.isDirectory()) {
                    Serial.print(entry.name());
                    Serial.print('\t');
                    Serial.println(entry.size());
                }
                entry.close();
            }
            dir.close();
        }
        Serial.println(F("END"));

        if (wasLogging) resume();
    }

    // Stream a file to Serial in BEGIN:/END framing.
    // Pass the date portion only: e.g. "20260612" → opens "<devId>/<devId>_20260612.csv"
    bool downloadFile(const char* datePart) {
        bool wasLogging = _logging;
        if (wasLogging) pause();

        char folder[8];
        sprintf(folder, "%04X", _deviceId);
        char path[28];
        if (datePart[0] != '\0') {
            sprintf(path, "%s/%s_%s.csv", folder, folder, datePart);
        } else {
            sprintf(path, "%s/%s_NODATE.csv", folder, folder);
        }

        File f = SD.open(path);
        bool ok = false;
        if (!f) {
            Serial.print(F("ERR: not found: ")); Serial.println(path);
        } else {
            Serial.print(F("BEGIN:")); Serial.println(path);
            while (f.available()) Serial.write(f.read());
            f.close();
            Serial.println(F("END"));
            ok = true;
        }

        if (wasLogging) resume();
        return ok;
    }

    const char* currentFilePath() const { return _filePath; }

private:
    uint8_t   _csPin;
    RtcClock& _rtc;
    uint16_t  _deviceId;
    uint16_t  _aircraftId;
    bool      _ready;
    bool      _logging;

    File      _logFile;
    char      _filePath[28];
    char      _currentDate[9];  // "YYYYMMDD" or ""

    unsigned long _lastFlushMs;
    unsigned long _lastRotateMs;
    uint32_t _framesLogged;
    uint32_t _framesDropped;

    void _openLogFile() {
        char date[9];
        _rtc.getDateString(date);
        strncpy(_currentDate, date, sizeof(_currentDate));

        char folder[8];
        sprintf(folder, "%04X", _deviceId);
        if (!SD.exists(folder)) SD.mkdir(folder);

        if (_currentDate[0] == '\0') {
            sprintf(_filePath, "%s/%s_NODATE.csv", folder, folder);
        } else {
            sprintf(_filePath, "%s/%s_%s.csv", folder, folder, _currentDate);
        }

        bool needHeader = !SD.exists(_filePath);
        _logFile = SD.open(_filePath, FILE_WRITE);
        if (!_logFile) { _logging = false; return; }

        if (needHeader) {
            _logFile.println(F("aircraft_id,timestamp,can_id,len,b0,b1,b2,b3,b4,b5,b6,b7"));
        }

        _logging     = true;
        _lastFlushMs = millis();
    }

    void _closeLogFile() {
        if (_logging && _logFile) {
            _logFile.flush();
            _logFile.close();
            _logging = false;
        }
    }

    void _checkRotation() {
        char date[9];
        _rtc.getDateString(date);
        if (strcmp(date, _currentDate) != 0) {
            _closeLogFile();
            _currentDate[0] = '\0';
            _openLogFile();
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
        f.println(F(" CanbusLogger startup"));
        f.close();
    }
};

} // namespace CanLogger

#endif // SD_LOGGER_H
