# Canbus Nano boars v1.0

This library handler the interfacing for the Arduino nano canbusboard. The Canbus board
has two i2c connectors, hardware SPI connectors for CANbus and a secondary SPI, a stepper motor driver
and a simple buzzer.

## CANbus driver

The Can bus is a hardware module, the MCP2515 CAN shield. This shield is open source and
there are several suppliers. port 10 (pin out 28) is used for the Chip Select (CS) on the 
MCP2515 shield. 
The CS for Canbus and the CS for the secondary SPI should not be active at the same time.

## Secondary SPI

A secondary SPI device can be added using a 5 pin header. The pin layout for this header 
is the same as the MAX6675 board. The CS is port 2 (pin out 20).  
The CS for Canbus and the CS for the secondary SPI should not be active at the same time.

## I2C ports

The two i2c ports have been terminated with a 4k7 resistor.

## Stepper motor driver

The stepper driver is integrated on the PCB and can be used to drive a low-yield
stepper motor, e.g. a stepper for rotating a needle (X27189)

## External power

The PCB can be powered directly using a 12V power source, or via the CAN bus:

- DB9: pin 3 for GND 
- DB9: pin 9 for 7 to 12V

The power circuit on the PCB is used to create the proper working voltage (VCC).
The external power is directly routed to the Vin on the Arduino NANO board
so it is safe to add the USB port at the same time that the external power is
connected.

(This is a MOD on the canboard V1, where VCC is connected to the 5V pin.)
