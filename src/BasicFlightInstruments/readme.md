# Basic flight instruments

This is a hardware module that contains a set of basic flight instruments.
It is an arduino nano based module.
Current setup is a single MAX6675 and an I2C pressure sensor (to be used as altimeter)

Current CS pins for MAX6675 are:

T1_CS : D2
SCK   : D13
SO    : D12

If multiple MAX6675 are linked, they will use D5 to D8 as T1_CS.

This hardware module has the same 8.000 Canbus breakout board as the CanMock module

A buzzer is connected to A0

## Memory usage (build 5, Arduino Nano ATmega328P)

| Resource | Used | Total | Percentage |
|---|---|---|---|
| Flash | 14 522 bytes | 30 720 bytes | 47.3% |
| RAM (static) | 511 bytes | 2 048 bytes | 25.0% |

## Future extensions

A future extension will be a dynamic pressure sensor (analog) connected to A1.