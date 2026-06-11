#pragma once
#include <EventManager.h>

#define CAN_ERROR_INVALID_COMMAND 0x01
#define CAN_ERROR_DATA_LOST 0x02
#define CAN_ERROR_ZERO_LENGTH 0x03
#define CAN_ERROR_INIT_FAILED 0x04

// CanSensor class is used as a wrapper for the Canbus library
// It is used to send sensor data over the Canbus network
// Each sensor has a unique identifier
namespace Canbus {
    class CanSensor {   
        public:
            CanSensor();
            bool begin(uint8_t identifier, byte clockSpeed, bool startupMessage);
            bool initialized();
            bool errorOccured();
            void loop();
            byte sendSensorInteger(unsigned char dataType, int data);
            byte sendSensorFloat(unsigned char dataType, float data);
            byte sendSensorString(unsigned char dataType, char* data);
            byte sendFailureCode(int code);
            byte sendCommand(unsigned char commandType, uint8_t remoteSensorId, char* data);
            byte sendMemValue(int address, int data);
            int getInterval();
            int setInterval(int interval);
            unsigned char* readMessage();
            EventManager eventManager;
        private:
            // Identifier for the sensor
            // It is typically set using DIP switches
            // on at compile time
            unsigned long sensorId;
            int interval = 100;
            bool initFailed = false;
            bool error = false;
            unsigned long lastTimerEvent = 0;
            void handleCommand(unsigned char* msg);            
    };

    // Define the clockspeed for the Canbus chip
    enum ClockSpeed{
        Clock20MHZ = 0x00,
        Clock16MHZ = 0x01,
        Clock8MHZ = 0x02,
    };

    enum messageTypes{
        kMessageTypeInteger = 'I',
        kMessageTypeFloat = 'F',
        kMessageTypeString = 'S',
        kMessageTypeCommand = 'C',
        kMessageTypeFailure = 'E',
        kMessageMemValue = 'M',
        kMessageInitialized = 0xBB,
    };    
}