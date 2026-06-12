# EMS-App

This folder will contain the windows/linux application that connects to the canbusmonitor using
the serial USB connector. It is used for detailed analysis of the messages, for configuring the system
and for extracting the data logs.

## Coding

The application is written in C#, and it should run on linux or windows. User interaction
is done through a web portal, which uses Reakt. Default port for the http traffic is 6502
(respecting the processor that helped me into programming)

## Main functionality

- Live logging
- System config (set time, system name for the log files)
- Extract logs
- Cleaning up logs on the canbus monitor