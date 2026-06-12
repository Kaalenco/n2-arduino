#ifndef ROTARY_CONTROL_H
#define ROTARY_CONTROL_H

#include <Arduino.h>

// Polling-based 3-button rotary encoder breakout (S1 / S2 / KEY).
// S1 = CW step, S2 = CCW step, KEY = push button.  All pins active-low
// with internal pull-ups.  Call poll() every loop iteration.
// getStep() returns -1, 0, or +1 since the last poll.
// wasButtonPressed() returns true once on the falling edge of KEY.
// Both values reset at the start of the next poll() call.

namespace CanMonitor {

class RotaryControl {
public:
    RotaryControl(uint8_t pinS1, uint8_t pinS2, uint8_t pinKey)
        : _pinS1(pinS1), _pinS2(pinS2), _pinKey(pinKey),
          _lastS1(HIGH), _lastS2(HIGH), _lastKey(HIGH),
          _step(0), _btnPressed(false) {}

    void begin() {
        pinMode(_pinS1,  INPUT_PULLUP);
        pinMode(_pinS2,  INPUT_PULLUP);
        pinMode(_pinKey, INPUT_PULLUP);
        _lastS1  = digitalRead(_pinS1);
        _lastS2  = digitalRead(_pinS2);
        _lastKey = digitalRead(_pinKey);
    }

    void poll() {
        _step       = 0;
        _btnPressed = false;

        uint8_t s1 = digitalRead(_pinS1);
        if (s1 == LOW && _lastS1 == HIGH) {
            _step = 1;
        }
        _lastS1 = s1;

        uint8_t s2 = digitalRead(_pinS2);
        if (s2 == LOW && _lastS2 == HIGH) {
            _step = -1;
        }
        _lastS2 = s2;

        uint8_t key = digitalRead(_pinKey);
        if (key == LOW && _lastKey == HIGH) {
            _btnPressed = true;
        }
        _lastKey = key;
    }

    int8_t getStep() const          { return _step; }
    bool   wasButtonPressed() const { return _btnPressed; }

private:
    uint8_t _pinS1, _pinS2, _pinKey;
    uint8_t _lastS1, _lastS2, _lastKey;
    int8_t  _step;
    bool    _btnPressed;
};

} // namespace CanMonitor

#endif // ROTARY_CONTROL_H
