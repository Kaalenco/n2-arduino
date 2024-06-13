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
        rotaryState = digitalRead(rotaryA); 
        if (rotaryState != lastRotaryState){     
            if (digitalRead(rotaryB) != rotaryState) 
            { 
                rotaryCounter ++;
                if (!eventManager.queueEvent(EventManager::EventType::kEventUser0, rotaryCounter)) {

                    return false;
                }
            } 
            else 
            {
                rotaryCounter --;
                if (!eventManager.queueEvent(EventManager::EventType::kEventUser1, rotaryCounter)){
                    return false;                
                }
            }
        } 
        lastRotaryState = rotaryState; // Updates the previous state of the outputA with the current state     
        return true;   
    }

    int RotaryEncoder::RotaryValue() {
        return rotaryCounter;
    }

}