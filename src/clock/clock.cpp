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

        if (! RTC.initialized() || RTC.lostPower()) {
            Serial.println("RTC is NOT initialized, set the time!");
            // When time needs to be set on a new device, or after a power loss, the
            // following line sets the RTC to the date & time this sketch was compiled
            RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
            // This line sets the RTC with an explicit date & time, for example to set
            // January 21, 2014 at 3am you would call:
            // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
            //
            // Note: allow 2 seconds after inserting battery or applying external power
            // without battery before calling adjust(). This gives the PCF8523's
            // crystal oscillator time to stabilize. If you call adjust() very quickly
            // after the RTC is powered, lostPower() may still return true.
        }

        // When the RTC was stopped and stays connected to the battery, it has
        // to be restarted by clearing the STOP bit. Let's do this to ensure
        // the RTC is running.
        RTC.start();

        currentTime = RTC.now();      
    }

    void RtcClock::ReadTime(){
        currentTime = RTC.now();
        hour = currentTime.hour();
        minute = currentTime.minute();
        second = currentTime.second();
        hms = (long)hour * 10000L + (long)minute * 100L + (long)second;
        Serial.println("Time: " + String(hms));
    }

    void RtcClock::Loop(){
        unsigned long currentMillis = millis();
        // Timer event every second
        if(currentMillis - lastTimerEvent > 1000)
        {
            // Serial.println("Timer event");
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