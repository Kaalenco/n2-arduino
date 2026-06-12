#ifndef ROTARY_CONTROL_H
#define ROTARY_CONTROL_H

#include <Arduino.h>

// Polling-based KY-040 quadrature rotary encoder (CLK/S1, DT/S2, KEY).
// Direction is decoded by sampling S2 (DT) at the moment S1 (CLK) rises:
//   S2 LOW  → CW  (+1)
//   S2 HIGH → CCW (-1)
// A cooldown on S1 prevents contact bounce from registering extra steps.
// Call poll() every loop iteration.
// getStep() returns -1, 0, or +1 since the last poll.
// wasButtonPressed() returns true once on the falling edge of KEY.
// Both values reset at the start of the next poll() call.

namespace CanMonitor {

class RotaryControl {
public:
    RotaryControl(uint8_t pinS1, uint8_t pinS2, uint8_t pinKey)
        : _pinS1(pinS1), _pinS2(pinS2), _pinKey(pinKey),
          _lastS1(HIGH), _s1CooldownMs(0),
          _rawKey(HIGH), _stableKey(HIGH), _keyChangeMs(0),
          _step(0), _btnPressed(false) {}

    void begin() {
        pinMode(_pinS1,  INPUT_PULLUP);
        pinMode(_pinS2,  INPUT_PULLUP);
        pinMode(_pinKey, INPUT_PULLUP);
        _lastS1       = digitalRead(_pinS1);
        _s1CooldownMs = 0;
        _rawKey       = digitalRead(_pinKey);
        _stableKey    = _rawKey;
        _keyChangeMs  = 0;
    }

    void poll() {
        _step       = 0;
        _btnPressed = false;
        unsigned long now = millis();

        // Quadrature decode: trigger on S1 (CLK) rising edge, sample S2 (DT) for direction.
        // At the rising edge: DT LOW → CW (+1), DT HIGH → CCW (-1).
        uint8_t s1 = digitalRead(_pinS1);
        if (s1 == HIGH && _lastS1 == LOW && (now - _s1CooldownMs) >= STEP_DEBOUNCE_MS) {
            _step         = (digitalRead(_pinS2) == LOW) ? 1 : -1;
            _s1CooldownMs = now;
        }
        _lastS1 = s1;

        // KEY: commit a state change only after it has been stable for
        // KEY_DEBOUNCE_MS consecutive milliseconds.
        uint8_t raw = digitalRead(_pinKey);
        if (raw != _rawKey) {
            _rawKey      = raw;
            _keyChangeMs = now;
        }
        if (_rawKey != _stableKey && (now - _keyChangeMs) >= KEY_DEBOUNCE_MS) {
            _stableKey = _rawKey;
            if (_stableKey == LOW) _btnPressed = true;  // falling edge = press
        }
    }

    int8_t getStep()          const { return _step; }
    bool   wasButtonPressed() const { return _btnPressed; }
    bool   isButtonDown()     const { return _stableKey == LOW; }

private:
    static const uint8_t  KEY_DEBOUNCE_MS  = 20;
    static const uint8_t  STEP_DEBOUNCE_MS = 50;

    uint8_t        _pinS1, _pinS2, _pinKey;
    uint8_t        _lastS1;
    unsigned long  _s1CooldownMs;
    uint8_t        _rawKey, _stableKey;
    unsigned long  _keyChangeMs;
    int8_t         _step;
    bool           _btnPressed;
};

} // namespace CanMonitor

#endif // ROTARY_CONTROL_H
