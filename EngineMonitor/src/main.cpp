#include <Arduino.h>
#include <EMSMemoryMap.h>
#include <PinsMap.h>
#include <TemperatureManager.h>
#include <OilPressureManager.h>
#include <SerialLogger.h>
#include <EngineDataLogger.h>
#include <EepromConfigure.h>
#include "processCanbus.h"

// Create logger instance for debugging output
SerialLogger logger(9600, true);  // 9600 baud, verbose mode

// Create CAN bus logger for engine data
CanbusLogging::EngineDataLogger canLogger(PIN_SPI_CS);

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
  logger.logPrint(F("  Sensors configured: "));
  logger.logPrintln(TemperatureManager::getSensorCount());
  logger.logPrint(F("  Sensors initialized: "));
  logger.logPrintln(sensorsInitialized);

  // List configured sensors
  logger.logMessage(F(""));
  logger.logMessage(F("Configured sensors:"));
  for (uint8_t i = 0; i < TemperatureManager::getSensorCount(); i++) {
    const TemperatureManager::SensorConfig* config = TemperatureManager::getSensorConfig(i);
    if (config != nullptr) {
      logger.logPrint(F("  [0x"));
      if (config->sensorId < 0x10) logger.logPrint('0');
      logger.logPrint(config->sensorId, HEX);
      logger.logPrint(F("] "));
      logger.logPrint(config->name);
      logger.logPrint(F(" (CS: D"));
      logger.logPrint(config->chipSelectPin);
      logger.logPrint(F(", Offset: "));
      logger.logPrint(config->temperatureOffset, 1);
      logger.logPrintln(F("°C)"));
    }
  }

  logger.logMessage(F(""));
  logger.logMessage(F("Ready for temperature monitoring..."));
  logger.logMessage(F(""));

  // Initialize oil pressure sensor
  logger.logMessage(F("Initializing oil pressure sensor..."));
  if (OilPressureManager::initialize()) {
    logger.logMessage(F("Oil pressure sensor initialized successfully"));
  } else {
    logger.logMessage(F("Oil pressure sensor initialization FAILED"));
    logger.logMessage(F("  error status: "));
    logger.logPrintln(OilPressureSensor::getStatus());
    logger.logMessage(F(""));
  }
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
  processCanMessages(canLogger, eepromConfig);

  // Create array to hold sensor readings
  Common::SensorReading readings[TemperatureManager::MAX_TEMP_SENSORS];

  // Read all temperature sensors
  uint8_t readCount = 0;
  readCount = TemperatureManager::readAll(readings, readCount, TemperatureManager::MAX_TEMP_SENSORS);

  // Read oil pressure sensor
  OilPressureManager::read(readings, readCount);
  readCount++;

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
        canLogger.setEGT1(readings[i].value, readings[i].warning );
        break;
      case SENSOR_TEMPERATURE_CHT_1:
        canLogger.setCHT1(readings[i].value, readings[i].warning );
        break;
      case SENSOR_TEMPERATURE_AMB:
        canLogger.setEngineAmbient(readings[i].value, false);
        break;
      case SENSOR_TEMPERATURE_OIL:
        canLogger.setOilTemp(readings[i].value, readings[i].warning);
        break;
      case SENSOR_PRESSURE_OIL:
        canLogger.setOilPressure(readings[i].value, readings[i].warning);
        break;
      default:
        // Unknown sensor ID - ignore
        break;
    }
  }

  // Send CAN frame
  if (canLogger.isReady()) {
    canLogger.sendFrame();
  }

  // Wait for next conversion cycle
  delay(1000);  // 1 second for easier reading
}


