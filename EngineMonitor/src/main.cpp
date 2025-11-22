#include <Arduino.h>
#include <EMSMemoryMap.h>
#include <PinsMap.h>
#include <TemperatureManager.h>
#include <SerialLogger.h>
#include <EngineDataLogger.h>
#include <EepromConfigure.h>

// Create logger instance for debugging output
SerialLogger logger(9600, true);  // 9600 baud, verbose mode

// Create CAN bus logger for engine data
CanbusLogging::EngineDataLogger canLogger(PIN_CANBUS);

// EEPROM configuration handler (initialized in setup after CAN bus is ready)
EepromConfig::EepromConfigure* eepromConfig = nullptr;


void setup() {
  // Initialize logger
  logger.begin();

  SPI.begin();
  
  // Signal to host computer that application has started
  logger.logMessage(F("ENGINE_MONITOR_STARTED"));
  logger.logMessage(F("EngineMonitor v1.0 - System Initialized"));
  logger.logMessage(F(""));

  // Initialize temperature sensors
  logger.logMessage(F("Initializing temperature sensors..."));
  uint8_t sensorsInitialized = TemperatureManager::initialize();

  logger.logMessage(F("Temperature sensor initialization:"));
  Serial.print(F("  Sensors configured: "));
  Serial.println(TemperatureManager::getSensorCount());
  Serial.print(F("  Sensors initialized: "));
  Serial.println(sensorsInitialized);

  // List configured sensors
  logger.logMessage(F(""));
  logger.logMessage(F("Configured sensors:"));
  for (uint8_t i = 0; i < TemperatureManager::getSensorCount(); i++) {
    const TemperatureManager::SensorConfig* config = TemperatureManager::getSensorConfig(i);
    if (config != nullptr) {
      Serial.print(F("  [0x"));
      if (config->sensorId < 0x10) Serial.print('0');
      Serial.print(config->sensorId, HEX);
      Serial.print(F("] "));
      Serial.print(config->name);
      Serial.print(F(" (CS: D"));
      Serial.print(config->chipSelectPin);
      Serial.print(F(", Offset: "));
      Serial.print(config->temperatureOffset, 1);
      Serial.println(F("°C)"));
    }
  }

  logger.logMessage(F(""));
  logger.logMessage(F("Ready for temperature monitoring..."));
  logger.logMessage(F(""));

  // Initialize CAN bus logger
  logger.logMessage(F("Initializing CAN bus..."));
  if (canLogger.begin(CAN_500KBPS, MCP_8MHZ)) {
    logger.logMessage(F("CAN bus initialized successfully"));

    // Initialize EEPROM configuration handler
    eepromConfig = new EepromConfig::EepromConfigure(canLogger.getCan());
    eepromConfig->begin();
    logger.logMessage(F("EEPROM configuration handler ready"));
  } else {
    logger.logMessage(F("CAN bus initialization FAILED"));
  }
  logger.logMessage(F(""));
}

void loop() {
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
        Serial.print(F("Config msg processed: 0x"));
        Serial.println(rxId, HEX);
      }
    }
  }

  // Create array to hold sensor readings
  TemperatureManager::SensorReading readings[TemperatureManager::MAX_TEMP_SENSORS];

  // Read all temperature sensors
  uint8_t readCount = TemperatureManager::readAll(readings, TemperatureManager::MAX_TEMP_SENSORS);

  // Log readings to serial terminal
  logger.beginFrame();
  logger.logReadings(readings, readCount);
  logger.endFrame();

  // Populate CAN logger with sensor data
  canLogger.clearData();

  for (uint8_t i = 0; i < readCount; i++) {
    if (!readings[i].success) continue;

    switch (readings[i].id) {
      case SENSOR_TEMPERATURE_EGT_1:
        canLogger.setEGT1(readings[i].celsius, readings[i].celsius > DEFAULT_EGT_WARNING);
        break;
      case SENSOR_TEMPERATURE_CHT_1:
        canLogger.setCHT1(readings[i].celsius, readings[i].celsius > DEFAULT_CHT_WARNING);
        break;
      case SENSOR_TEMPERATURE_AMB:
        canLogger.setEngineAmbient(readings[i].celsius, false);
        break;
      // Note: Oil temperature sensor would need to be added to TemperatureManager
      // canLogger.setOilTemp(oilTemp, oilTemp > DEFAULT_OIL_TEMP_WARNING);
    }
  }

  // TODO: Read oil pressure from analog pin
  // int oilPressureRaw = analogRead(PIN_ANALOG_OIL_PRESSURE);
  // uint8_t oilPressureBar = mapOilPressure(oilPressureRaw);
  // canLogger.setOilPressure(oilPressureBar, oilPressureBar < MIN_OIL_PRESSURE);

  // Send CAN frame
  if (canLogger.isReady()) {
    canLogger.sendFrame();
  }

  // Wait for next conversion cycle
  delay(1000);  // 1 second for easier reading
}


