#pragma once
#include "RTCLib.h"
#include <EventManager.h>

using namespace std;

namespace Clock {
    
    // Enumeration for date/time components
    enum DateTimeComponent {
        YEAR_2DIGIT = 0,  // 2-digit year (00-99)
        MONTH = 1,        // Month (1-12)
        DAY = 2,          // Day (1-31)
        HOUR = 3,         // Hour (0-23)
        MINUTE = 4,       // Minute (0-59)
        SECOND = 5        // Second (0-59)
    };
    
    class RtcClock
    {
        public:
            bool RtcFound=0;
            DateTime currentTime;
            EventManager eventManager;
            int hour = 0;
            int minute = 0;
            int second = 0;
            long hms = 0;

            RtcClock();
            ~RtcClock();

            void Begin();
            void ReadTime();
            void Loop();
            
            // Date/Time configuration functions
            void SetDateTimeComponent(DateTimeComponent component, int value);
            String GetFormattedDateTime(); // Returns ISO format: YYMMDDTHHmmss
        private:
            /* data */
            /* RTC_DS1307 RTC; */
            RTC_PCF8523  RTC;
            unsigned long lastTimerEvent = 0;
            
            // Configuration storage
            int configYear = 24;    // 2-digit year (default: 2024)
            int configMonth = 1;    // Month (1-12)
            int configDay = 1;      // Day (1-31)
            int configHour = 0;     // Hour (0-23)
            int configMinute = 0;   // Minute (0-59)
            int configSecond = 0;   // Second (0-59)
            
            // Helper functions
            int ClampValue(int value, int min, int max);
            int GetDaysInMonth(int year, int month);
            void ApplyConfiguration();
    };
}