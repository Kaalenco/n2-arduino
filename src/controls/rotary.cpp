#include <Arduino.h>
#include "controls.h"

using namespace std;

namespace Controls {

    RotaryEncoder::RotaryEncoder() {
        rotaryA = INT8_MAX;
        rotaryB = INT8_MAX;
        ButtonControl::pin = INT8_MAX;
    }

    RotaryEncoder::RotaryEncoder(uint8_t pinA, uint8_t pinB)
    {
        rotaryA = pinA;
        rotaryB = pinB;
        ButtonControl::pin = INT8_MAX;
    }

    RotaryEncoder::RotaryEncoder(uint8_t pinA, uint8_t pinB, uint8_t pinKey)
    {
        rotaryA = pinA;
        rotaryB = pinB;
        ButtonControl::pin = pinKey;
    }

    RotaryEncoder::RotaryEncoder(uint8_t pinA, uint8_t pinB, uint8_t pinKey, int debounceDelay)
    {
        rotaryA = pinA;
        rotaryB = pinB;
        ButtonControl::pin = pinKey;
        ButtonControl::debounceDelay = debounceDelay;
    }

    RotaryEncoder::~RotaryEncoder() {
        rotaryA = INT8_MAX;
        rotaryB = INT8_MAX;
        ButtonControl::pin = INT8_MAX;
    }

    void RotaryEncoder::Begin() {
        lastRotaryState = 0;
        if(rotaryA!=INT8_MAX && rotaryB!=INT8_MAX) {
            pinMode(rotaryA, INPUT_PULLUP);
            pinMode(rotaryB, INPUT_PULLUP);
            lastRotaryState = digitalRead(rotaryA);
        }
        rotaryCounter = 0;

        if(pin==INT8_MAX) return;
        pinMode(pin, INPUT_PULLUP);
        ButtonControl::Reset();
    }

    bool RotaryEncoder::Loop() { 
        bool result = true;
        if(rotaryA!=INT8_MAX && rotaryB!=INT8_MAX) {
            if (!LoopEncoder()) {
                result = false;
            }
        }
        if (!ButtonControl::Loop()) {
            result = false;
        }
        return result;
    }

    bool RotaryEncoder::LoopEncoder() { 
        unsigned long currentMillis = millis();
        bool rotaryState = (digitalRead(rotaryA)==0);
        if (rotaryState != lastRotaryState) {
            lastRotaryDebounceTime = millis();
            lastRotaryDebounceState = rotaryState;
        }

        if ((currentMillis - lastDebounceTime) > debounceDelay) {
            if (rotaryState != lastRotaryState){     
                bool rotaryBState = (digitalRead(rotaryB)==0);
                if (rotaryBState != rotaryState) 
                { 
                    rotaryCounter ++;
                } 
                else 
                {
                    rotaryCounter --;
                }

                // Raise an event only when stable (both pins at the same state)
                if(rotaryState && rotaryCounter != lastRotaryCounter)
                {
                    lastRotaryCounter = rotaryCounter;
                    if (!eventManager.queueEvent(EventManager::EventType::kEventMenu0, rotaryCounter)) 
                    {
                        return false;
                    }
                }
                lastRotaryState = rotaryState;   
            } 
        }

        
        return true;   
    }

    int RotaryEncoder::RotaryValue() {
        return rotaryCounter;
    }

}