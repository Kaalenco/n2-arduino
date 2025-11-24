// processCanbus.h - CAN bus message processing for EEPROM configuration
// Handles incoming CAN messages and routes them to appropriate handlers

#ifndef PROCESS_CANBUS_H
#define PROCESS_CANBUS_H

#include <Arduino.h>
#include <EngineDataLogger.h>
#include <EepromConfigure.h>
#include <SerialLogger.h>

// External logger instance defined in main.cpp
extern SerialLogger logger;

/**
 * Process incoming CAN messages for EEPROM configuration
 *
 * @param canLogger Reference to the CAN bus logger
 * @param eepromConfig Pointer to the EEPROM configuration handler
 */
void processCanMessages(CanbusLogging::EngineDataLogger& canLogger,
                        EepromConfig::EepromConfigure* eepromConfig);

#endif // PROCESS_CANBUS_H
