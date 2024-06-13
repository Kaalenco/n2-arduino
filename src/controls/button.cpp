#include <Arduino.h>
#include "controls.h"

using namespace std;

namespace Controls {

    ButtonControl::ButtonControl() {
        this->pin = UINT8_MAX;
        debounceDelay = DEFAULT_DEBOUNCE_DELAY;
    }

    ButtonControl::ButtonControl(uint8_t pin) {
        this->pin = pin;
        debounceDelay = DEFAULT_DEBOUNCE_DELAY;
    }

    ButtonControl::ButtonControl(uint8_t pin, int debounceDelay) {
        this->pin = pin;
        this->debounceDelay = debounceDelay;
    }

    ButtonControl::~ButtonControl() {
        pin = UINT8_MAX;
        debounceDelay = DEFAULT_DEBOUNCE_DELAY;
    }

    void ButtonControl::Begin() {
        if(pin == UINT8_MAX) return;
        pinMode(pin, INPUT_PULLUP);
        Reset();
    }

    void ButtonControl::Reset() {
        errorCode = 0;
        lastDebounceTime = 0;
        lastState = LOW;
        buttonState = LOW;
    }

    bool ButtonControl::ButtonState() {
        return buttonState;
    }

    int ButtonControl::ErrorCode() {
        return errorCode;
    }

    bool ButtonControl::Loop() {
        if(pin == UINT8_MAX) return true;

        unsigned long currentMillis = millis();
        bool keyDown = (digitalRead(pin)==0);
        if (keyDown != lastState) {
            lastDebounceTime = millis();
            lastState = keyDown;
        }

        if ((currentMillis - lastDebounceTime) > debounceDelay) {
            if (keyDown) 
            {
                if(!buttonState) {
                    buttonState = true;
                    if (!eventManager.queueEvent(EventManager::EventType::kEventKeyPress, pin)) {
                        errorCode = 1;
                        return false;
                    }
                }
            } 
            else 
            {
                if(buttonState) {
                    buttonState = false;
                    if (!eventManager.queueEvent(EventManager::EventType::kEventKeyRelease, pin)) {
                        errorCode = 2;
                        return false;
                    }
                }
            }
        }
        return true;
    }
}
        