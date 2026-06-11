#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <CanBusMCP2515.h>
#include <TachLogger.h>
#include <RpmSensor.h>
#include <EepromConfigure.h>
#include <PinsMap.h>
#include <TachMemoryMap.h>

static const unsigned long MEASURE_INTERVAL_MS = 1000;

static const uint16_t CAN_ID_LOOPBACK_TEST  = 0x7FF;
static const uint16_t CAN_ID_SYSTEM_INIT    = 0x7F0;
static const uint8_t  SYSTEM_TYPE_CODE      = 0x01;  // TachometerInstrument
static const uint8_t  LOOPBACK_TEST_DATA[4] = { 0xA5, 0x5A, 0x42, 0x01 };

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);
CanBusMCP2515 can;
TachLogging::TachLogger tachLogger(can);
EepromConfig::EepromConfigure* eepromConfig = nullptr;

uint8_t  pulsesPerRev;
uint16_t maxRpm;
uint16_t canId;

uint16_t currentRpm = 0;
bool     overSpeed  = false;
unsigned long lastMeasureMs = 0;


void loadConfig() {
    if (EEPROM.read(EEPROM_TACH_MAGIC) != EEPROM_TACH_MAGIC_VALUE) {
        EEPROM.update(EEPROM_TACH_MAGIC, EEPROM_TACH_MAGIC_VALUE);
        EEPROM.update(EEPROM_TACH_PULSES_PER_REV, TACH_DEFAULT_PULSES_PER_REV);
        uint16_t defMax = TACH_DEFAULT_MAX_RPM;
        uint16_t defId  = TACH_DEFAULT_CAN_ID;
        EEPROM.put(EEPROM_TACH_MAX_RPM, defMax);
        EEPROM.put(EEPROM_TACH_CAN_ID,  defId);
    }

    pulsesPerRev = EEPROM.read(EEPROM_TACH_PULSES_PER_REV);
    EEPROM.get(EEPROM_TACH_MAX_RPM, maxRpm);
    EEPROM.get(EEPROM_TACH_CAN_ID,  canId);

    if (pulsesPerRev == 0 || pulsesPerRev > 10) pulsesPerRev = TACH_DEFAULT_PULSES_PER_REV;
    if (maxRpm == 0 || maxRpm > 9999)           maxRpm       = TACH_DEFAULT_MAX_RPM;
    if (canId  == 0 || canId  > 0x7FF)          canId        = TACH_DEFAULT_CAN_ID;
}

// Display layout (128×64):
//   y=0–11  : status banner — "RPM" label or inverted over-speed warning
//   y=13    : separator line
//   y=18–41 : RPM number, size 3 (18×24px per char), centered horizontally
//   y=55–63 : "Max: XXXX" in size 1

void updateDisplay() {
    display.clearDisplay();

    // Status banner
    if (overSpeed) {
        display.fillRect(0, 0, OLED_WIDTH, 12, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
        display.setTextSize(1);
        display.setCursor(4, 2);
        display.print(F("!! OVER SPEED !!"));
        display.setTextColor(SSD1306_WHITE);
    } else {
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1);
        display.setCursor(4, 2);
        display.print(F("RPM"));
    }

    // Separator line
    display.drawFastHLine(0, 13, OLED_WIDTH, SSD1306_WHITE);

    // Large RPM number, centered
    char rpmStr[6];
    sprintf(rpmStr, "%u", currentRpm);
    uint8_t textW = strlen(rpmStr) * 18;  // size 3: 6px base × 3 = 18px per char
    display.setTextSize(3);
    display.setCursor((OLED_WIDTH - textW) / 2, 18);
    display.print(rpmStr);

    // Bottom line: max RPM
    char maxStr[14];
    sprintf(maxStr, "Max: %u", maxRpm);
    display.setTextSize(1);
    display.setCursor(0, 55);
    display.print(maxStr);

    display.display();
}

bool runCanStartup() {
    if (can.begin(CanBusInterface::SPEED_500KBPS, CanBusInterface::MODE_LOOPBACK) != CanBusInterface::OK) {
        Serial.println(F("CAN loopback init FAILED"));
        return false;
    }

    CanBusInterface::Message testMsg;
    testMsg.id       = CAN_ID_LOOPBACK_TEST;
    testMsg.length   = 4;
    testMsg.extended = false;
    testMsg.rtr      = false;
    memcpy(testMsg.data, LOOPBACK_TEST_DATA, 4);
    memset(testMsg.data + 4, 0, 4);

    if (can.sendMessage(testMsg) != CanBusInterface::OK) {
        Serial.println(F("CAN loopback send FAILED"));
        return false;
    }

    delay(10);

    CanBusInterface::Message rxMsg;
    if (can.receiveMessage(rxMsg, 50) != CanBusInterface::OK ||
        rxMsg.id != CAN_ID_LOOPBACK_TEST || rxMsg.length != 4 ||
        memcmp(rxMsg.data, LOOPBACK_TEST_DATA, 4) != 0) {
        Serial.println(F("CAN loopback verify FAILED"));
        return false;
    }

    if (can.setMode(CanBusInterface::MODE_NORMAL) != CanBusInterface::OK) {
        Serial.println(F("CAN mode switch FAILED"));
        return false;
    }

    CanBusInterface::Message initMsg;
    initMsg.id       = CAN_ID_SYSTEM_INIT;
    initMsg.length   = 3;
    initMsg.extended = false;
    initMsg.rtr      = false;
    initMsg.data[0]  = SYSTEM_TYPE_CODE;
    initMsg.data[1]  = canId & 0xFF;
    initMsg.data[2]  = (canId >> 8) & 0xFF;
    memset(initMsg.data + 3, 0, 5);
    can.sendMessage(initMsg);

    return true;
}

void setup() {
    Serial.begin(9600);
    Wire.begin();
    SPI.begin();

    loadConfig();

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
        Serial.println(F("OLED init FAILED"));
    }
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(0, 16);
    display.print(F("Tachometer"));
    display.setTextSize(1);
    display.setCursor(0, 40);
    display.print(F("Initializing..."));
    display.display();

    RpmSensor::begin(PIN_RPM_PULSE, pulsesPerRev, RPM_PULSE_TRIGGER);
    lastMeasureMs = millis();

    Serial.println(F("TACHOMETER_STARTED"));
    Serial.print(F("Pulses/rev: ")); Serial.println(pulsesPerRev);
    Serial.print(F("Max RPM:    ")); Serial.println(maxRpm);
    Serial.print(F("CAN ID:     0x")); Serial.println(canId, HEX);

    if (runCanStartup()) {
        Serial.println(F("CAN bus OK"));
        eepromConfig = new EepromConfig::EepromConfigure(can);
        eepromConfig->begin();
    } else {
        Serial.println(F("CAN bus FAILED"));
    }

    updateDisplay();
}

void loop() {
    if (can.isReady() && can.messageAvailable()) {
        CanBusInterface::Message msg;
        if (can.receiveMessage(msg) == CanBusInterface::OK && eepromConfig != nullptr) {
            eepromConfig->processMessage(msg);
        }
    }

    unsigned long now = millis();
    if (now - lastMeasureMs >= MEASURE_INTERVAL_MS) {
        unsigned long elapsed = now - lastMeasureMs;
        lastMeasureMs = now;

        currentRpm = RpmSensor::readRpm(elapsed);
        overSpeed  = (currentRpm > maxRpm);

        updateDisplay();

        if (tachLogger.isReady()) {
            tachLogger.send(canId, currentRpm, overSpeed);
        }
    }
}
