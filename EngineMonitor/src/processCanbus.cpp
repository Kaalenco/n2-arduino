// processCanbus.cpp - CAN bus message processing for EEPROM configuration

#include "processCanbus.h"

void processCanMessages(CanbusLogging::EngineDataLogger& canLogger,
                        EepromConfig::EepromConfigure* eepromConfig) {
  // Process incoming CAN messages for EEPROM configuration
  if (eepromConfig != nullptr && canLogger.isReady()) {
    // Check for incoming CAN messages
    CanBusInterface::Message msg;
    while (canLogger.getCan().messageAvailable()) {
      CanBusInterface::Result result = canLogger.getCan().receiveMessage(msg, 100);

      if (result == CanBusInterface::OK) {
        // Process configuration messages
        if (eepromConfig->processMessage(msg.id, msg.length, msg.data)) {
          // Message was a configuration message and was processed
          logger.logPrint(F("Config msg processed: 0x"));
          logger.logPrintln(msg.id, HEX);
        }
      }
    }
  }
}
