#ifndef RTC_CLOCK_H
#define RTC_CLOCK_H

#include <Arduino.h>
#include <RTClib.h>

namespace CanLogger {

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

    bool setFromUnix(uint32_t unixTime) {
        if (!_rtcFound) return false;
        _rtc.adjust(DateTime(unixTime));
        _timeSet = true;
        return true;
    }

    // "YYYYMMDD" for file naming, or "" if time not set.
    void getDateString(char* buf8) const {
        if (!_timeSet) { buf8[0] = '\0'; return; }
        DateTime now = _rtc.now();
        sprintf(buf8, "%04u%02u%02u", now.year(), now.month(), now.day());
    }

    // "YYYY-MM-DD HH:MM:SS" or "T+<ms>" if time not set.
    void getTimestamp(char* buf20) const {
        if (_timeSet) {
            DateTime now = _rtc.now();
            sprintf(buf20, "%04u-%02u-%02u %02u:%02u:%02u",
                    now.year(), now.month(), now.day(),
                    now.hour(), now.minute(), now.second());
        } else {
            sprintf(buf20, "T+%lu", millis());
        }
    }

private:
    RTC_DS1307 _rtc;
    bool _rtcFound;
    bool _timeSet;
};

} // namespace CanLogger

#endif // RTC_CLOCK_H
