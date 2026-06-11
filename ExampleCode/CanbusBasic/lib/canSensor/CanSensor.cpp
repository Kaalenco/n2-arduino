#include <Arduino.h>
#include <SPI.h>
#include <mcp_can.h>
#include <EventManager.h>
#include <EepromMap.h>
#include <Commands.h>
#include <Queue.h>
#include <EEPROM.h>
#include "CanSensor.h"

namespace Canbus {

    const int SPI_CS_PIN = 10;
    MCP_CAN CAN(SPI_CS_PIN); // Set CS pin

    fifoQueue::Queue fifo;

    byte sendInteger(int id, unsigned char messageType, unsigned char dataType, int data)
    {
        unsigned char msg[4] = {
            messageType,
            dataType,
            (uint8_t)(data >> 8),
            (uint8_t)data
        };
        return CAN.sendMsgBuf(id, 0, 4, msg);
    }

    CanSensor::CanSensor() {       
    }

    bool CanSensor::begin(uint8_t identifier, byte clockSpeed, bool startupMessage)
    {        
        // Read the interval from the EEPROM
        EEPROM.get(EEPROM_GAUGE_ZERO_OFFSET, interval);
        if(interval <= 0) {
            interval = random(200, 800);
        }

        // Initialize
        int retryCount = 0;
        this->sensorId = 0x100 | (identifier & 0x0F);
        
        if(startupMessage){
            Serial.println(F("Starting CAN"));
            Serial.print(F("Id       : "));
            Serial.println(sensorId);
            Serial.print(F("Interval : "));
            Serial.println(interval);
        }

        // Initialize the CAN bus using the correct clock (16 / 8)
        while (CAN_OK != CAN.begin(MCP_ANY, CAN_100KBPS, clockSpeed) && retryCount<10) { 
            retryCount++;
            if(startupMessage)
                Serial.println(" Retry " + String(retryCount));
            delay(300);
        }

        // Change to normal mode to allow messages to be transmitted
        CAN.setMode(MCP_NORMAL);

        if(retryCount>=10)
        {
            if(startupMessage)
                Serial.println(F("CAN failed!"));
            initFailed = true;
            eventManager.queueEvent(EventManager::EventType::kEventCanDataError, CAN_ERROR_INIT_FAILED);
        }
        else
        {          
            if(startupMessage)
                Serial.println(F("CAN 100KBPS"));
            unsigned char data[5] = { 0xAA, 'H', 'E', 'L', 'O'};
            // send data:  id = 0x00, standrad frame, data len = 4, data: data buf
            CAN.sendMsgBuf(sensorId, 0, 5, data);
        }
        delay(100);
        return !initFailed;
    }

    bool CanSensor::initialized() 
    {
        return !initFailed;
    }

    bool CanSensor::errorOccured()
    {
        return error;
    }

    int CanSensor::getInterval()
    {
        return interval;
    }

    int CanSensor::setInterval(int interval)
    {
        int oldInterval = this->interval;
        this->interval = interval;
        EEPROM.put(EEPROM_CAN_INTERVAL, interval);
        return oldInterval;
    }

    byte CanSensor::sendSensorInteger(unsigned char dataType, int data)
    {
        return sendInteger(sensorId, kMessageTypeInteger, dataType, data);
    }

    byte CanSensor::sendMemValue(int address, int data)
    {
        unsigned char msg[5] = {
            kMessageMemValue,
            (uint8_t)(address >> 8),
            (uint8_t)address,
            (uint8_t)(data >> 8),
            (uint8_t)data
        };

        // Memory data, 5 bytes
        return CAN.sendMsgBuf(sensorId, 0, 5, msg);
    }

/*
Float has size of 4 bytes. Therefore you need 4 bytes in your array 
to store each float. If you want to pass 4 floats you need 16 bytes

You can cast a pointer to float to a byte array:

   float a = 12.34
   byte dataArray[4] = {
      ((uint8_t*)&a)[0],
      ((uint8_t*)&a)[1],
      ((uint8_t*)&a)[2],
      ((uint8_t*)&a)[3]
   };

and make opposite operation on the receiver side:

    float a;
    ((uint8_t*)&a)[0] = dataArray[0];
    ((uint8_t*)&a)[1] = dataArray[1];
    ((uint8_t*)&a)[2] = dataArray[2];
    ((uint8_t*)&a)[3] = dataArray[3];
*/

    byte CanSensor::sendSensorString(unsigned char dataType, char* data)
    {
        unsigned char msg[8] = {
            kMessageTypeString,
            dataType,
            '\0',
            '\0',
            '\0',
            '\0',
            '\0',
            '\0'
        };

        // Copy the data to the message
        for (size_t i = 0; i < 5; i++)
        {
            if(data[i] == '\0')
            {
                break;
            }
            msg[i + 2] = data[i];
        }        

        // Normal message, 8 bytes (maximum size)
        return CAN.sendMsgBuf(sensorId, 0, 8, msg);
    }

    byte CanSensor::sendSensorFloat(unsigned char dataType, float data)
    {
        unsigned char msg[6] = {
            kMessageTypeFloat,
            dataType,
            ((uint8_t*)&data)[0],
            ((uint8_t*)&data)[1],
            ((uint8_t*)&data)[2],
            ((uint8_t*)&data)[3]    
        };

        // Normal message, 6 bytes
        return CAN.sendMsgBuf(sensorId, 0, 6, msg);
    }

    byte CanSensor::sendCommand(unsigned char commandType, uint8_t remoteSensorId, char* data)
    {
        unsigned char msg[8] = {
            kMessageTypeCommand,
            remoteSensorId,
            '\0',
            '\0',
            '\0',
            '\0',
            '\0',
            '\0'
        };

        // Copy the data to the message
        for (size_t i = 0; i < 5; i++)
        {
            msg[i + 2] = data[i];
        }        

        // Normal message, 8 bytes (maximum size)
        return CAN.sendMsgBuf(sensorId, 0, 8, msg);
    }

    byte CanSensor::sendFailureCode(int code)
    {
        return sendInteger(sensorId, 0xEE, 0, code);
    }

    void CanSensor::loop() 
    {
        // prevent the loop from running if the sensor was not initialized
        if(initFailed)
        {
            return;
        }

        unsigned long currentMillis = millis();
        // Do something every interval
        if(currentMillis - lastTimerEvent > 100)
        {
            lastTimerEvent = currentMillis;

            unsigned char msg[8];
            unsigned char len = 8;
            unsigned long canId = 0;            
            unsigned long rxId;
            bool extended = false;
            bool remoteRequest = false;

            // Do your stuff here
            int frameReceived = CAN.checkReceive();
            // Result value is 3 or 4, CAN_MSGAVAIL or CAN_NOMSG
            // Both values are positive
            if (frameReceived == CAN_MSGAVAIL) 
            {
                memset(msg, 0, 8);
                CAN.readMsgBuf(&rxId, &len, msg);

                if((rxId & 0x80000000) == 0x80000000)     // Determine if ID is standard (11 bits) or extended (29 bits)
                {
                    extended = true;
                    canId = (rxId & 0x1FFFFFFF);
                }
                else
                {
                    extended = false;
                    canId = rxId;
                }

                if(extended == false && canId != sensorId)
                {
                    if((rxId & 0x40000000) == 0x40000000)
                    {
                        remoteRequest = true;
                    }
                    else
                    {
                        remoteRequest = false;
                    }

                    if (remoteRequest) {
                        eventManager.queueEvent(EventManager::EventType::kEventRequestData, 0);
                    } 
                    else if (len > 0) 
                    {
                        if(msg[0] == kMessageTypeCommand)
                        {
                            // if no address, the command is invalid
                            if (len>1)
                            {
                                unsigned long checkSensor = 0x100 | (msg[1] & 0x0F);
                                if(checkSensor == sensorId)
                                {
                                    unsigned char data[7];
                                    for (unsigned char i = 0; i < len-2; i++)
                                    {
                                        data[i] = msg[i+2];
                                    }
                                    handleCommand(data);
                                }
                                // else ignore the command
                            }                            
                            else
                            {
                                // invalid command, no remoteId
                                eventManager.queueEvent(EventManager::EventType::kEventCanDataError, CAN_ERROR_INVALID_COMMAND);
                            }
                        }        
                        else
                        {
                            // push the data on the stack
                            // if the stack is overflown, send an event
                            fifo.push(msg);
                            eventManager.queueEvent(EventManager::EventType::kEventCanDataReceived, fifo.count());
                            if(fifo.overFlow()){
                                eventManager.queueEvent(EventManager::EventType::kEventCanDataError, CAN_ERROR_DATA_LOST);
                            }                        
                        }   
                    }           
                    else
                    {
                        eventManager.queueEvent(EventManager::EventType::kEventCanDataError, CAN_ERROR_ZERO_LENGTH);
                    }                    
                    delay(100);
                }            
            }
        }
    }

    unsigned char* CanSensor::readMessage()
    {
        static unsigned char data[8];
        if(fifo.pop(data))
        {
            return data;
        }
        return NULL;
    }

    void CanSensor::handleCommand(unsigned char* msg)
    {
        // handle the command message
        // msg[0] is the command type
        Serial.print(F("RVC: "));
        Serial.println(msg[0]);
        if(msg[0] == SET_SETTING_VALUE)
        {
            // set the interval
            int address = (msg[1] << 8) | msg[2];
            int newValue = (msg[3] << 8) | msg[4];
            EEPROM.put(address, newValue);
            return;
        }
        if(msg[0] == GET_SETTING_VALUE)
        {
            // get the interval
            int address = (msg[1] << 8) | msg[2];
            int value;
            EEPROM.get(address, value);
            sendMemValue(address, value);
            return;
        }
    }
}