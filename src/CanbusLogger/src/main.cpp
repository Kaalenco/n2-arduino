#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <EEPROM.h>
#include <CanBusMCP2515.h>
#include <RtcClock.h>
#include <SdLogger.h>
#include <PinsMap.h>
#include <CanbusLoggerMemoryMap.h>
#include "version.h"

static const unsigned long PING_INTERVAL_MS = 30000;

static const CanBusInterface::Speed SPEED_TABLE[] = {
    CanBusInterface::SPEED_125KBPS,
    CanBusInterface::SPEED_250KBPS,
    CanBusInterface::SPEED_500KBPS,
};

CanBusMCP2515        can;
CanLogger::RtcClock  rtcClock;
CanLogger::SdLogger  sdLogger(PIN_SD_CS, rtcClock, CLG_DEFAULT_DEVICE_ID, CLG_DEFAULT_AIRCRAFT_ID);

uint8_t  canSpeedIndex;
uint16_t deviceId;
uint16_t aircraftId;
bool     canError = false;

unsigned long lastPingMs = 0;

// ---------------------------------------------------------------------------

void loadConfig() {
    if (EEPROM.read(EEPROM_CLG_MAGIC) != EEPROM_CLG_MAGIC_VALUE) {
        EEPROM.update(EEPROM_CLG_MAGIC,     EEPROM_CLG_MAGIC_VALUE);
        EEPROM.update(EEPROM_CLG_CAN_SPEED, CLG_DEFAULT_CAN_SPEED);
        uint16_t defDev  = CLG_DEFAULT_DEVICE_ID;
        uint16_t defAcft = CLG_DEFAULT_AIRCRAFT_ID;
        EEPROM.put(EEPROM_CLG_DEVICE_ID,   defDev);
        EEPROM.put(EEPROM_CLG_AIRCRAFT_ID, defAcft);
    }
    canSpeedIndex = EEPROM.read(EEPROM_CLG_CAN_SPEED);
    if (canSpeedIndex > 2) canSpeedIndex = CLG_DEFAULT_CAN_SPEED;
    EEPROM.get(EEPROM_CLG_DEVICE_ID,   deviceId);
    EEPROM.get(EEPROM_CLG_AIRCRAFT_ID, aircraftId);
    if (deviceId == 0) deviceId = CLG_DEFAULT_DEVICE_ID;
}

bool runCanStartup() {
    if (can.begin(SPEED_TABLE[canSpeedIndex], CanBusInterface::MODE_LISTEN_ONLY) != CanBusInterface::OK) {
        Serial.println(F("CAN init FAILED"));
        return false;
    }
    static const char* labels[] = { "125", "250", "500" };
    Serial.print(F("CAN OK listen-only "));
    Serial.print(labels[canSpeedIndex]);
    Serial.println(F(" kbps"));
    return true;
}

// ---------------------------------------------------------------------------

static bool parseHex16(const char* s, uint16_t& out) {
    if (strlen(s) != 4) return false;
    char* end;
    unsigned long v = strtoul(s, &end, 16);
    if (*end != '\0') return false;
    out = (uint16_t)v;
    return true;
}

void sendPing() {
    char ts[20];
    rtcClock.getTimestamp(ts);

    Serial.print(F("PING can="));
    Serial.print(canError ? F("ERR") : (can.isReady() ? F("OK") : F("INIT")));
    Serial.print(F(" logging="));
    Serial.print(sdLogger.isLogging() ? F("YES") : F("NO"));
    Serial.print(F(" frames="));
    Serial.print(sdLogger.framesLogged());
    Serial.print(F(" dropped="));
    Serial.print(sdLogger.framesDropped());
    Serial.print(F(" rtc="));
    Serial.println(rtcClock.isSet() ? ts : "none");
}

void processSerial() {
    if (!Serial.available()) return;
    char cmd[48];
    uint8_t len = (uint8_t)Serial.readBytesUntil('\n', cmd, sizeof(cmd) - 1);
    while (len > 0 && (cmd[len - 1] == '\r' || cmd[len - 1] == ' ')) len--;
    cmd[len] = '\0';

    if (strncmp_P(cmd, PSTR("TIME:"), 5) == 0) {
        uint32_t unixTime = strtoul(cmd + 5, nullptr, 10);
        if (unixTime < 1000000000UL) {
            Serial.println(F("ERR: Unix timestamp expected (>1000000000)"));
            return;
        }
        if (rtcClock.setFromUnix(unixTime)) {
            sdLogger.onTimeSet();
            Serial.println(F("RTC set."));
        } else {
            Serial.println(F("ERR: RTC not found"));
        }

    } else if (strncmp_P(cmd, PSTR("DEVID:"), 6) == 0) {
        uint16_t id;
        if (!parseHex16(cmd + 6, id) || id == 0) {
            Serial.println(F("ERR: DEVID expects 4-digit hex, non-zero"));
            return;
        }
        EEPROM.put(EEPROM_CLG_DEVICE_ID, id);
        deviceId = id;
        sdLogger.setDeviceId(id);
        Serial.print(F("Device ID set to 0x")); Serial.println(id, HEX);

    } else if (strncmp_P(cmd, PSTR("AIRCRAFT:"), 9) == 0) {
        uint16_t id;
        if (!parseHex16(cmd + 9, id)) {
            Serial.println(F("ERR: AIRCRAFT expects 4-digit hex"));
            return;
        }
        EEPROM.put(EEPROM_CLG_AIRCRAFT_ID, id);
        aircraftId = id;
        sdLogger.setAircraftId(id);
        Serial.print(F("Aircraft ID set to 0x")); Serial.println(id, HEX);

    } else if (strncmp_P(cmd, PSTR("SPEED:"), 6) == 0) {
        int speed = atoi(cmd + 6);
        uint8_t idx;
        switch (speed) {
            case 125: idx = 0; break;
            case 250: idx = 1; break;
            case 500: idx = 2; break;
            default:
                Serial.println(F("ERR: use 125, 250 or 500"));
                return;
        }
        EEPROM.update(EEPROM_CLG_CAN_SPEED, idx);
        canSpeedIndex = idx;
        Serial.print(F("CAN speed set to ")); Serial.print(speed);
        Serial.println(F(" kbps. Restart to apply."));

    } else if (strcmp_P(cmd, PSTR("STATUS")) == 0) {
        sendPing();
        Serial.print(F("File: "));
        Serial.println(sdLogger.isLogging() ? sdLogger.currentFilePath() : "(none)");
        Serial.print(F("Device ID:   0x")); Serial.println(deviceId,   HEX);
        Serial.print(F("Aircraft ID: 0x")); Serial.println(aircraftId, HEX);

    } else if (strcmp_P(cmd, PSTR("PAUSE")) == 0) {
        sdLogger.pause();
        Serial.println(F("Logging paused."));

    } else if (strcmp_P(cmd, PSTR("RESUME")) == 0) {
        sdLogger.resume();
        Serial.println(sdLogger.isLogging() ? F("Logging resumed.") : F("ERR: could not open log file."));

    } else if (strcmp_P(cmd, PSTR("LISTFILES")) == 0) {
        if (!sdLogger.isReady()) { Serial.println(F("ERR: SD not available")); return; }
        sdLogger.listFiles();

    } else if (strncmp_P(cmd, PSTR("DOWNLOAD:"), 9) == 0) {
        if (!sdLogger.isReady()) { Serial.println(F("ERR: SD not available")); return; }
        sdLogger.downloadFile(cmd + 9);

    } else if (strcmp_P(cmd, PSTR("RESET")) == 0) {
        Serial.println(F("Resetting..."));
        Serial.flush();
        void (*reset)() = nullptr;
        reset();

    } else {
        Serial.println(F("Commands: TIME:<unix>  DEVID:<hex4>  AIRCRAFT:<hex4>  SPEED:<kbps>  STATUS  PAUSE  RESUME  LISTFILES  DOWNLOAD:<date>  RESET"));
    }
}

// ---------------------------------------------------------------------------

void setup() {
    Serial.begin(115200);
    Wire.begin();
    SPI.begin();

    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, LOW);

    loadConfig();

    bool rtcFound = rtcClock.begin();
    if (!rtcFound) {
        Serial.println(F("RTC not found - using relative timestamps"));
    } else if (!rtcClock.isSet()) {
        Serial.println(F("RTC found but not set - send TIME:<unix>"));
    } else {
        Serial.println(F("RTC OK"));
    }

    // Patch the sdLogger with the loaded IDs before begin() opens any files.
    sdLogger.setDeviceId(deviceId);
    sdLogger.setAircraftId(aircraftId);

    bool sdOk = sdLogger.begin();
    if (!sdOk) {
        Serial.println(F("SD init FAILED"));
    } else {
        Serial.println(F("SD OK"));
    }

    Serial.println(F("CANBUS_LOGGER_STARTED"));
    Serial.print(F("Firmware: v"));
    Serial.print(FW_MAJOR); Serial.print('.'); Serial.print(FW_MINOR); Serial.print('.');
    Serial.println(FW_BUILD);

    canError = !runCanStartup();

    Serial.print(F("Device ID:   0x")); Serial.println(deviceId,   HEX);
    Serial.print(F("Aircraft ID: 0x")); Serial.println(aircraftId, HEX);

    digitalWrite(PIN_STATUS_LED, sdLogger.isLogging() && !canError ? HIGH : LOW);

    unsigned long now = millis();
    lastPingMs = now;
    sendPing();
}

void loop() {
    if (!canError && can.messageAvailable()) {
        CanBusInterface::Message msg;
        if (can.receiveMessage(msg, 50) == CanBusInterface::OK) {
            sdLogger.log(msg);
        }
    }

    sdLogger.tick();
    processSerial();

    unsigned long now = millis();
    if (now - lastPingMs >= PING_INTERVAL_MS) {
        lastPingMs = now;
        sendPing();
    }
}
