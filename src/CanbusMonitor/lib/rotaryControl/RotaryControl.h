#ifndef ROTARY_CONTROL_H
#define ROTARY_CONTROL_H

#include <Arduino.h>

// Polling-based rotary encoder with integrated push button.
// Call poll() every loop iteration. getStep() returns -1, 0, or +1 since
// the last poll. wasButtonPressed() returns true once on the falling edge
// of the button pin. Both values are reset on the next poll() call.

namespace CanMonitor {

class RotaryControl {
public:
    RotaryControl(uint8_t pinClk, uint8_t pinDt, uint8_t pinBtn)
        : _pinClk(pinClk), _pinDt(pinDt), _pinBtn(pinBtn),
          _lastClk(HIGH), _lastBtn(HIGH), _step(0), _btnPressed(false) {}

    void begin() {
        pinMode(_pinClk, INPUT_PULLUP);
        pinMode(_pinDt,  INPUT_PULLUP);
        pinMode(_pinBtn, INPUT_PULLUP);
        _lastClk = digitalRead(_pinClk);
        _lastBtn = digitalRead(_pinBtn);
    }

    void poll() {
        _step       = 0;
        _btnPressed = false;

        uint8_t clk = digitalRead(_pinClk);
        if (clk != _lastClk) {
            _lastClk = clk;
            if (clk == LOW) {
                _step = (digitalRead(_pinDt) == HIGH) ? 1 : -1;
            }
        }

        uint8_t btn = digitalRead(_pinBtn);
        if (btn == LOW && _lastBtn == HIGH) {
            _btnPressed = true;
        }
        _lastBtn = btn;
    }

    int8_t getStep() const         { return _step; }
    bool   wasButtonPressed() const { return _btnPressed; }

private:
    uint8_t _pinClk, _pinDt, _pinBtn;
    uint8_t _lastClk, _lastBtn;
    int8_t  _step;
    bool    _btnPressed;
};

} // namespace CanMonitor

#endif // ROTARY_CONTROL_H
