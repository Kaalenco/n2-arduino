#pragma once
#include <EventManager.h>

// Max6675Sensor class is used as a wrapper for the Max6675 library
// It is used to read sensor data and send it to the EventManager
// Each sensor has a unique identifier
namespace Max6675Sensor {
    class Max6675Temperature {   
        public:
            Max6675Temperature(int sensorId);
            bool initialized();
            bool errorOccured();
            void loop();
            EventManager eventManager;
        private:
            // Identifier for the sensor
            // It is typically set using DIP switches
            // on at compile time
            unsigned long sensorId;
            bool initFailed = false;
            bool error = false;
            unsigned long lastTimerEvent = 0;
    };

}