#include <Arduino.h>
#include <Wire.h>
#include "clock.h"

using namespace std;

namespace Clock {
        
    void RtcClock::Begin(){
        lastTimerEvent = millis();

        if(RTC.begin())
        {
            RtcFound=1;
        }
        else
        {
            RtcFound=0;
        }

        if(!RTC.isrunning()){
            RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
        }
        currentTime = RTC.now();      
    }

    void RtcClock::ReadTime(){
        currentTime = RTC.now();
        hour = currentTime.hour();
        minute = currentTime.minute();
        second = currentTime.second();
        hms = hour * 10000 + minute * 100 + second;
        Serial.println("Time: " + String(hms));
    }

    void RtcClock::Loop(){
        unsigned long currentMillis = millis();
        if(currentMillis - lastTimerEvent > 1000)
        {
            Serial.println("Timer event");
            lastTimerEvent = currentMillis;
            if(!eventManager.queueEvent(EventManager::EventType::kEventTimer0, 0)){
                Serial.println("Failed to queue timer event");            
            };
        }
    }

    RtcClock::RtcClock()
    {
    }
    
    RtcClock::~RtcClock()
    {
    }    
}