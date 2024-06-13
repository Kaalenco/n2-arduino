#pragma once
#include "RTCLib.h"
#include <EventManager.h>

using namespace std;

namespace Clock {
    
    class RtcClock
    {
        public:
            bool RtcFound=0;
            DateTime currentTime;
            EventManager eventManager;
            int hour = 0;
            int minute = 0;
            int second = 0;
            int hms = 0;

            RtcClock();
            ~RtcClock();

            void Begin();
            void ReadTime();
            void Loop();
        private:
            /* data */
            RTC_DS1307 RTC;
            unsigned long lastTimerEvent = 0;
    };
}