# 00004. Low power library

2024-06-14

## Status

__New__

Update [00005.Low power library for AVR](.\00005-low-power-library-for-avr)

## Context

Switching to a low power at the end of each cycle (loop) could help save energy. The device could even be set to a low power mode if
it has little or no activity on its input.

## Decision

Go to low power mode at the end of each cycle using the "ArduinoLowPower" library.

## Consequences

See [arduino.cc](https://docs.arduino.cc/learn/electronics/low-power/) for more information about using the low power library.
Use [platformio.org](https://registry.platformio.org/libraries/arduino-libraries/Arduino%20Low%20Power/installation) for configuring the libary for use.


