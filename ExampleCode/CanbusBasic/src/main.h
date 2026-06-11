#pragma once
#include <Arduino.h>
#include <EventManager.h>
#include <CanSensor.h>
#include <GaugeDisplay.h>
#include <SensorTypes.h>
#include <string.h>
#include <EepromMap.h>

unsigned long lastTimerEvent = 0;
// Define the canbus object
Canbus::CanSensor canSensor;

// Default setup and loop
void setup();
void loop();

// Event manager loops
void handleEvents();
void raiseEvents();

// Canbus event handlers
void remoteRequest(int event, int param);
void dataLost(int event, int param);
void dataReceived(int event, int param);