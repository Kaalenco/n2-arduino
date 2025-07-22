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
        
        // Initialize configuration with current RTC time
        configYear = currentTime.year() - 2000; // Convert to 2-digit year
        configMonth = currentTime.month();
        configDay = currentTime.day();
        configHour = currentTime.hour();
        configMinute = currentTime.minute();
        configSecond = currentTime.second();      
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

    // ========================================
    // DATE/TIME CONFIGURATION FUNCTIONS
    // ========================================

    void RtcClock::SetDateTimeComponent(DateTimeComponent component, int value) {
        switch(component) {
            case YEAR_2DIGIT:
                configYear = ClampValue(value, 0, 99);
                break;
            case MONTH:
                configMonth = ClampValue(value, 1, 12);
                // Adjust day if current day is invalid for new month
                configDay = ClampValue(configDay, 1, GetDaysInMonth(2000 + configYear, configMonth));
                break;
            case DAY:
                configDay = ClampValue(value, 1, GetDaysInMonth(2000 + configYear, configMonth));
                break;
            case HOUR:
                configHour = ClampValue(value, 0, 23);
                break;
            case MINUTE:
                configMinute = ClampValue(value, 0, 59);
                break;
            case SECOND:
                configSecond = ClampValue(value, 0, 59);
                break;
        }
        
        // Apply the new configuration to the RTC
        ApplyConfiguration();
        
        Serial.println("DateTime updated: " + GetFormattedDateTime());
    }

    String RtcClock::GetFormattedDateTime() {
        // Format: YYMMDDTHHmmss
        String result = "";
        
        // Add 2-digit year
        if(configYear < 10) result += "0";
        result += String(configYear);
        
        // Add month
        if(configMonth < 10) result += "0";
        result += String(configMonth);
        
        // Add day
        if(configDay < 10) result += "0";
        result += String(configDay);
        
        // Add T separator
        result += "T";
        
        // Add hour
        if(configHour < 10) result += "0";
        result += String(configHour);
        
        // Add minute
        if(configMinute < 10) result += "0";
        result += String(configMinute);
        
        // Add second
        if(configSecond < 10) result += "0";
        result += String(configSecond);
        
        return result;
    }

    // ========================================
    // HELPER FUNCTIONS
    // ========================================

    int RtcClock::ClampValue(int value, int min, int max) {
        if(value < min) return min;
        if(value > max) return max;
        return value;
    }

    int RtcClock::GetDaysInMonth(int year, int month) {
        switch(month) {
            case 1: case 3: case 5: case 7: case 8: case 10: case 12:
                return 31;
            case 4: case 6: case 9: case 11:
                return 30;
            case 2:
                // Check for leap year
                if((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
                    return 29;
                else
                    return 28;
            default:
                return 31; // Fallback
        }
    }

    void RtcClock::ApplyConfiguration() {
        if(RtcFound) {
            // Convert 2-digit year to 4-digit year (assuming 20xx)
            int fullYear = 2000 + configYear;
            
            DateTime newDateTime(fullYear, configMonth, configDay, 
                               configHour, configMinute, configSecond);
            
            RTC.adjust(newDateTime);
            currentTime = RTC.now();
            
            // Update the current time variables
            ReadTime();
        }
    }

    RtcClock::RtcClock()
    {
    }
    
    RtcClock::~RtcClock()
    {
    }    
}