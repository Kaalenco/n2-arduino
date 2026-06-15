#ifndef RTC_CLOCK_H
#define RTC_CLOCK_H

#include <Arduino.h>
#include <RTClib.h>

// DS1307 wrapper with relative-time fallback.
//
// When no RTC is present or before TIME: is received, timestamps are expressed
// as milliseconds since boot formatted as "T+<ms>".  Once setFromSerial() is
// called with a valid Unix timestamp the RTC is programmed and isSet() becomes
// true; subsequent calls to getTimestamp() return "YYYY-MM-DD HH:MM:SS".

namespace CanMonitor {

class RtcClock {
public:
    RtcClock() : _rtcFound(false), _timeSet(false) {}

    bool begin() {
        _rtcFound = _rtc.begin();
        if (_rtcFound && _rtc.isrunning()) {
            _timeSet = true;
        }
        return _rtcFound;
    }

    bool isFound() const { return _rtcFound; }
    bool isSet()   const { return _timeSet;  }

    // Set RTC from Unix timestamp (seconds since 1970-01-01 00:00:00 UTC).
    bool setFromUnix(uint32_t unixTime) {
        if (!_rtcFound) return false;
        _rtc.adjust(DateTime(unixTime));
        _timeSet = true;
        return true;
    }

    // Read current time components for interactive editing.
    bool getComponents(uint16_t& year, uint8_t& month, uint8_t& day,
                       uint8_t& hour, uint8_t& minute) {
        if (!_timeSet) return false;
        DateTime now = _rtc.now();
        year   = now.year();
        month  = now.month();
        day    = now.day();
        hour   = now.hour();
        minute = now.minute();
        return true;
    }

    // Set RTC from individual components; seconds are always reset to 0.
    bool setFromComponents(uint16_t year, uint8_t month, uint8_t day,
                           uint8_t hour, uint8_t minute) {
        if (!_rtcFound) return false;
        _rtc.adjust(DateTime(year, month, day, hour, minute, 0));
        _timeSet = true;
        return true;
    }

    // Returns current date as "YYYYMMDD" for file naming, or "" if time not set.
    void getDateString(char* buf8) {
        if (!_timeSet) {
            buf8[0] = '\0';
            return;
        }
        DateTime now = _rtc.now();
        sprintf(buf8, "%04u%02u%02u", now.year(), now.month(), now.day());
    }

    // Returns timestamp as "YYYY-MM-DD HH:MM:SS" or "T+<ms>" if time not set.
    void getTimestamp(char* buf20) {
        if (_timeSet) {
            DateTime now = _rtc.now();
            sprintf(buf20, "%04u-%02u-%02u %02u:%02u:%02u",
                    now.year(), now.month(), now.day(),
                    now.hour(), now.minute(), now.second());
        } else {
            sprintf(buf20, "T+%lu", millis());
        }
    }

    // Returns "HH:MM:SS" (buf must be ≥ 9 bytes), or "" if time not set.
    void getTimeDisplay(char* buf9) {
        if (!_timeSet) { buf9[0] = '\0'; return; }
        DateTime now = _rtc.now();
        sprintf(buf9, "%02u:%02u:%02u", now.hour(), now.minute(), now.second());
    }

    // Returns "DD-MM-YYYY" (buf must be ≥ 11 bytes), or "" if time not set.
    void getDateDisplay(char* buf11) {
        if (!_timeSet) { buf11[0] = '\0'; return; }
        DateTime now = _rtc.now();
        sprintf(buf11, "%02u-%02u-%04u", now.day(), now.month(), now.year());
    }

private:
    RTC_DS1307 _rtc;
    bool _rtcFound;
    bool _timeSet;
};

} // namespace CanMonitor

#endif // RTC_CLOCK_H
