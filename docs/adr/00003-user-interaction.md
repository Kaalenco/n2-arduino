# 00003. User interaction

2024-06-14

## Status

__New__

## Context

Embedded applications need interaction with its users using input and outpout devices. Specifically input devices pose a challenge
as users are much slower that computers and waiting for user input could slow down the application or make it unresponsive. In 
languages like C#, there is a possibility to control application flow using events. Using this in an embedded (arduino) environment
would be a possible solution for this issue.

## Decision

There are several options for implementing an event based program flow. Just browse the web for 'arduino event manager' and you will
find several options. The library used in this project is from Igor Miktor. The eventmanager object is defined for each class that 
facilitates events. Hooking up an event is done with static methods that bind an event from a class to functions, either static or
instance methods. 

## Consequences

In the 'loop' of the arduino application, a call for handling each eventmanager and a call for raising events is required. These calls
should be grouped within the methods `handleEvents()` and `raiseEvents()`. 

See [Arduino-EventManager on github](https://github.com/arduino-collections/arduino-EventManager/blob/master/README.md) for configuring the development environment.

This documentation is created using the (adr-cli tool)[https://github.com/gjkaal/adr-cli].
