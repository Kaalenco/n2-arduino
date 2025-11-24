// processCanbus.cpp - CAN bus message processing for EEPROM configuration

#include "processCanbus.h"
#include <mcp_can.h>

void processCanMessages(CanbusLogging::EngineDataLogger& canLogger,
                        EepromConfig::EepromConfigure* eepromConfig) {
  // Process incoming CAN messages for EEPROM configuration
  if (eepromConfig != nullptr && canLogger.isReady()) {
    // Check for incoming CAN messages
    while (canLogger.getCan().checkReceive() == CAN_MSGAVAIL) {
      uint32_t rxId;
      uint8_t len;
      uint8_t rxBuf[8];

      canLogger.getCan().readMsgBuf(&rxId, &len, rxBuf);

      // Process configuration messages
      if (eepromConfig->processMessage(rxId, len, rxBuf)) {
        // Message was a configuration message and was processed
        logger.logPrint(F("Config msg processed: 0x"));
        logger.logPrintln(rxId, HEX);
      }
    }
  }
}
