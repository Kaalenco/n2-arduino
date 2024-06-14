#pragma once
#include <Arduino.h>
#include <EventManager.h>

using namespace std;

namespace Controls
{

    #define DEFAULT_DEBOUNCE_DELAY 50

    class ButtonControl {
        protected:
            unsigned long debounceDelay;
            unsigned long lastDebounceTime;
            bool lastState;
            int errorCode;
            uint8_t pin = UINT8_MAX;
            bool buttonState;
        public:            
            EventManager eventManager;
            virtual void Begin();
            virtual bool Loop();

            virtual bool ButtonState();
            int ErrorCode();
            void Reset();

            ButtonControl();
            ButtonControl(uint8_t pin);
            ButtonControl(uint8_t pin, int debounceDelay);
            ~ButtonControl();
    };

    class RotaryEncoder : public ButtonControl {
        private:
            bool lastRotaryState;
            unsigned long lastRotaryDebounceTime;
            bool lastRotaryDebounceState;
            int lastRotaryCounter;
            int rotaryCounter;
            uint8_t rotaryA = UINT8_MAX;
            uint8_t rotaryB = UINT8_MAX;

            virtual bool LoopEncoder();
        public:
            virtual void Begin();
            virtual bool Loop();

            int RotaryValue();
            RotaryEncoder();
            RotaryEncoder(uint8_t pinA, uint8_t pinB);
            RotaryEncoder(uint8_t pinA, uint8_t pinB, uint8_t pinKey);
            RotaryEncoder(uint8_t pinA, uint8_t pinB, uint8_t pinKey, int debounceDelay);
            ~RotaryEncoder();
    };

    class KeypadControl {
        public:
            KeypadControl(uint8_t pin);
            KeypadControl(uint8_t pin, int debounceDelay);
            ~KeypadControl();

            void Begin();
            bool Loop();
            bool KeyAvailable();
            char PeekKey();
            char ReadKey();
            int ErrorCode();
            void Reset();
        private:
            unsigned long debounceDelay;
            unsigned long lastDebounceTime;
            bool lastState;
            int errorCode;
            uint8_t pin;
            char key;
    };
}