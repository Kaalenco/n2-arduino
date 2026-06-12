#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include <CanBusMCP2515.h>
#include <CanMessageStore.h>
#include <RotaryControl.h>
#include <RtcClock.h>
#include <SdLogger.h>
#include <PinsMap.h>
#include <CanMonitorMemoryMap.h>
#include "version.h"

static const unsigned long DISPLAY_REFRESH_MS = 250;
static const unsigned long BUS_TIMEOUT_MS     = 3000;
static const unsigned long PING_INTERVAL_MS   = 4000;

static const CanBusInterface::Speed SPEED_TABLE[] = {
    CanBusInterface::SPEED_125KBPS,
    CanBusInterface::SPEED_250KBPS,
    CanBusInterface::SPEED_500KBPS,
};

static const uint16_t CAN_ID_LOOPBACK_TEST  = 0x7FF;
static const uint16_t CAN_ID_SYSTEM_INIT    = 0x7F0;
static const uint16_t CAN_ID_CONFIG         = 0x7E0;
static const uint16_t CAN_ID_SYSRESET       = 0x7EF;
static const uint8_t  SYSTEM_TYPE_CODE      = 0x02;  // CanbusMonitor
static const uint8_t  LOOPBACK_TEST_DATA[4] = { 0xA5, 0x5A, 0x42, 0x01 };

// --- Known CAN ID display metadata -----------------------------------------

struct CanIdInfo {
    uint32_t id;
    char     mnemonic[4];  // 3 chars + null
    char     unit[5];      // up to 4 chars + null
    uint8_t  divisor;      // display value = raw / divisor (1 = direct)
};

// CanFIX parameter IDs — see docs/canfix/src/canfix.json
static const CanIdInfo CAN_INFO_TABLE[] = {
    { 512,  "RPM", "",      1   },  // N1/Engine RPM — UINT, direct
    { 1282, "EGT", "\001C", 10  },  // Exhaust Gas Temperature — UINT, x0.1 C
    { 1280, "CHT", "\001C", 10  },  // Cylinder Head Temperature — UINT, x0.1 C
    { 1030, "OAT", "\001C", 100 },  // Total Air Temperature — INT, x0.01 C
    { 1031, "IAT", "\001C", 100 },  // Static Air Temperature — INT, x0.01 C
    { 388,  "ALT", "ft",    1   },  // Indicated Altitude — DINT, ft (lower 2 bytes)
};
static const uint8_t CAN_INFO_COUNT = sizeof(CAN_INFO_TABLE) / sizeof(CAN_INFO_TABLE[0]);

static const CanIdInfo* findCanInfo(uint32_t id) {
    for (uint8_t i = 0; i < CAN_INFO_COUNT; i++) {
        if (CAN_INFO_TABLE[i].id == id) return &CAN_INFO_TABLE[i];
    }
    return nullptr;
}

// --- Warning thresholds (RAM; updated by SET commands) ----------------------

struct CanWarnState {
    uint32_t canId;
    uint16_t warnHi;  // warn "HI" if raw value >= warnHi (0 = disabled)
    uint16_t warnLo;  // warn "LO" if raw value <= warnLo (0 = disabled)
};

static CanWarnState warnTable[] = {
    { 512,  2800, 0 },  // RPM: default redline 2800
    { 1282, 1500, 0 },  // EGT: default caution high 150 C (raw 1500 in x0.1)
    { 1280, 1100, 0 },  // CHT: default caution high 110 C (raw 1100 in x0.1)
};
static const uint8_t WARN_TABLE_SIZE = sizeof(warnTable) / sizeof(warnTable[0]);

static CanWarnState* findWarnState(uint32_t id) {
    for (uint8_t i = 0; i < WARN_TABLE_SIZE; i++) {
        if (warnTable[i].canId == id) return &warnTable[i];
    }
    return nullptr;
}

LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);
CanBusMCP2515 can;
CanMonitor::CanMessageStore store;
CanMonitor::RotaryControl rotary(PIN_ENC_S1, PIN_ENC_S2, PIN_ENC_KEY);
CanMonitor::RtcClock rtcClock;

uint8_t   canSpeedIndex;
uint16_t  deviceId;
uint16_t  aircraftId;

CanMonitor::SdLogger* sdLogger = nullptr;

int8_t      selectedIndex  = 0;
bool        lcdBacklightOn = true;
bool        busActive      = false;
bool        busError       = false;
bool        sdReady        = false;
unsigned long lastMessageMs = 0;
unsigned long lastDisplayMs = 0;
unsigned long lastPingMs    = 0;

static bool hasAlert() {
    for (uint8_t i = 0; i < store.count(); i++) {
        const CanMonitor::CanEntry* e = store.getAt(i);
        if (!e) continue;
        const CanWarnState* ws = findWarnState(e->id);
        if (!ws) continue;
        uint16_t raw = e->data[0] | ((uint16_t)e->data[1] << 8);
        if ((ws->warnHi > 0 && raw >= ws->warnHi) || (ws->warnLo > 0 && raw <= ws->warnLo)) return true;
    }
    return false;
}

static uint8_t totalItems() {
    return store.count() + (rtcClock.isSet() ? 2 : 0);
}


void loadConfig() {
    if (EEPROM.read(EEPROM_CM_MAGIC) != EEPROM_CM_MAGIC_VALUE) {
        EEPROM.update(EEPROM_CM_MAGIC,     EEPROM_CM_MAGIC_VALUE);
        EEPROM.update(EEPROM_CM_CAN_SPEED, CM_DEFAULT_CAN_SPEED);
        uint16_t defDev  = CM_DEFAULT_DEVICE_ID;
        uint16_t defAcft = CM_DEFAULT_AIRCRAFT_ID;
        EEPROM.put(EEPROM_CM_DEVICE_ID,   defDev);
        EEPROM.put(EEPROM_CM_AIRCRAFT_ID, defAcft);
    }
    canSpeedIndex = EEPROM.read(EEPROM_CM_CAN_SPEED);
    if (canSpeedIndex > 2) canSpeedIndex = CM_DEFAULT_CAN_SPEED;
    EEPROM.get(EEPROM_CM_DEVICE_ID,   deviceId);
    EEPROM.get(EEPROM_CM_AIRCRAFT_ID, aircraftId);
    if (deviceId == 0) deviceId = CM_DEFAULT_DEVICE_ID;
}

void lcdRow(uint8_t row, const char* text) {
    char buf[LCD_COLS + 1];
    snprintf(buf, sizeof(buf), "%-16s", text);
    lcd.setCursor(0, row);
    lcd.print(buf);
}

void updateDisplay() {
    static uint8_t spinnerIdx = 0;
    static const uint8_t spinnerChars[] = {'|', '/', '-', 0};  // 0 = CGRAM backslash glyph

    // Clamp selection after store or RTC state changes (e.g. CLEAR command).
    uint8_t total = totalItems();
    if (total == 0) selectedIndex = 0;
    else if (selectedIndex >= (int8_t)total) selectedIndex = (int8_t)(total - 1);

    char line[LCD_COLS + 1];

    if (busError) {
        lcdRow(0, "BUS ERROR");
    } else if (busActive || store.count() > 0) {
        const char* stateLabel = busActive ? "ACT" : "TMO";
        snprintf(line, sizeof(line), "%-3s %2u %c %s %c",
                 stateLabel, store.count(),
                 hasAlert() ? '*' : ' ',
                 sdReady ? "SD" : "  ",
                 rtcClock.isSet() ? 'C' : ' ');
        lcdRow(0, line);
    } else {
        lcdRow(0, sdReady ? "WAITING... SD" : "WAITING...");
    }

    lcd.setCursor(15, 0);
    lcd.write(spinnerChars[spinnerIdx]);
    spinnerIdx = (spinnerIdx + 1) % 4;

    uint8_t canCount = store.count();
    if (selectedIndex < (int8_t)canCount) {
        const CanMonitor::CanEntry* entry = store.getAt((uint8_t)selectedIndex);
        if (entry == nullptr) {
            lcdRow(1, "No data");
        } else {
            const CanIdInfo*   info = findCanInfo(entry->id);
            const CanWarnState* ws  = findWarnState(entry->id);
            uint16_t raw = entry->data[0] | ((uint16_t)entry->data[1] << 8);

            const char* warn = "  ";
            if (ws) {
                if      (ws->warnHi > 0 && raw >= ws->warnHi) warn = "HI";
                else if (ws->warnLo > 0 && raw <= ws->warnLo) warn = "LO";
            }

            if (info == nullptr) {
                char valBuf[10];
                snprintf(valBuf, sizeof(valBuf), "%02X%02X %02X%02X",
                         entry->data[0], entry->data[1],
                         entry->data[2], entry->data[3]);
                snprintf(line, sizeof(line), "??? %-10s%-2s", valBuf, warn);
            } else {
                uint16_t displayVal = raw / info->divisor;
                if (info->unit[0] != '\0') {
                    snprintf(line, sizeof(line), "%-3s %6u %-3s%-2s",
                             info->mnemonic, displayVal, info->unit, warn);
                } else {
                    snprintf(line, sizeof(line), "%-3s %6u    %-2s",
                             info->mnemonic, displayVal, warn);
                }
            }
            lcdRow(1, line);
        }
    } else if (rtcClock.isSet()) {
        uint8_t rtcIdx = (uint8_t)selectedIndex - canCount;
        if (rtcIdx == 0) {
            char timeBuf[9];
            rtcClock.getTimeDisplay(timeBuf);
            snprintf(line, sizeof(line), "TIME %s", timeBuf);
        } else {
            char dateBuf[11];
            rtcClock.getDateDisplay(dateBuf);
            snprintf(line, sizeof(line), "DATE %s", dateBuf);
        }
        lcdRow(1, line);
    } else {
        lcdRow(1, "No data");
    }
}

static void sendBusReset() {
    CanBusInterface::Message msg;
    msg.id       = CAN_ID_SYSRESET;
    msg.length   = 0;
    msg.extended = false;
    msg.rtr      = false;
    memset(msg.data, 0, 8);
    can.sendMessage(msg);
}

static void sendConfig(uint8_t targetType, uint8_t paramId, uint16_t value) {
    CanBusInterface::Message msg;
    msg.id       = CAN_ID_CONFIG;
    msg.length   = 4;
    msg.extended = false;
    msg.rtr      = false;
    msg.data[0]  = targetType;
    msg.data[1]  = paramId;
    msg.data[2]  = value & 0xFF;
    msg.data[3]  = (value >> 8) & 0xFF;
    memset(msg.data + 4, 0, 4);
    can.sendMessage(msg);
}

static bool lookupConfig(const char* type, const char* param,
                         uint8_t& targetType, uint8_t& paramId) {
    if (strcmp_P(type, PSTR("RPM")) == 0) {
        targetType = 0x01;
        if (strcmp_P(param, PSTR("GRL")) == 0) { paramId = 0x01; return true; }
        if (strcmp_P(param, PSTR("GRH")) == 0) { paramId = 0x02; return true; }
        if (strcmp_P(param, PSTR("RED")) == 0) { paramId = 0x03; return true; }
    } else if (strcmp_P(type, PSTR("ALT")) == 0) {
        targetType = 0x02;
        if (strcmp_P(param, PSTR("QNH")) == 0) { paramId = 0x01; return true; }
    } else if (strcmp_P(type, PSTR("EGT")) == 0) {
        targetType = 0x03;
        if (strcmp_P(param, PSTR("CAL")) == 0) { paramId = 0x01; return true; }
        if (strcmp_P(param, PSTR("CAH")) == 0) { paramId = 0x02; return true; }
    } else if (strcmp_P(type, PSTR("CHT")) == 0) {
        targetType = 0x04;
        if (strcmp_P(param, PSTR("CAL")) == 0) { paramId = 0x01; return true; }
        if (strcmp_P(param, PSTR("CAH")) == 0) { paramId = 0x02; return true; }
    }
    return false;
}

static void applyLocalConfig(uint8_t targetType, uint8_t paramId, uint16_t value) {
    uint32_t canId = 0;
    bool isHi = false;
    if      (targetType == 0x01 && paramId == 0x03) { canId = 512;  isHi = true;  }
    else if (targetType == 0x03 && paramId == 0x02) { canId = 1282; isHi = true;  }
    else if (targetType == 0x03 && paramId == 0x01) { canId = 1282; isHi = false; }
    else if (targetType == 0x04 && paramId == 0x02) { canId = 1280; isHi = true;  }
    else if (targetType == 0x04 && paramId == 0x01) { canId = 1280; isHi = false; }
    if (canId == 0) return;
    CanWarnState* ws = findWarnState(canId);
    if (!ws) return;
    if (isHi) ws->warnHi = value; else ws->warnLo = value;
}

static bool parseHex16(const char* s, uint16_t& out) {
    if (strlen(s) != 4) return false;
    char* end;
    unsigned long v = strtoul(s, &end, 16);
    if (*end != '\0') return false;
    out = (uint16_t)v;
    return true;
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
            if (sdLogger) sdLogger->onTimeSet();
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
        EEPROM.put(EEPROM_CM_DEVICE_ID, id);
        deviceId = id;
        if (sdLogger) sdLogger->setDeviceId(id);
        Serial.print(F("Device ID set to 0x")); Serial.println(id, HEX);

    } else if (strncmp_P(cmd, PSTR("AIRCRAFT:"), 9) == 0) {
        uint16_t id;
        if (!parseHex16(cmd + 9, id)) {
            Serial.println(F("ERR: AIRCRAFT expects 4-digit hex"));
            return;
        }
        EEPROM.put(EEPROM_CM_AIRCRAFT_ID, id);
        aircraftId = id;
        if (sdLogger) sdLogger->setAircraftId(id);
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
        EEPROM.update(EEPROM_CM_CAN_SPEED, idx);
        canSpeedIndex = idx;
        Serial.print(F("CAN speed set to ")); Serial.print(speed);
        Serial.println(F(" kbps. Restart to apply."));

    } else if (strcmp_P(cmd, PSTR("CLEAR")) == 0) {
        store.clear();
        busActive = false;
        selectedIndex = 0;
        Serial.println(F("Store cleared."));

    } else if (strcmp_P(cmd, PSTR("LIST")) == 0) {
        if (store.count() == 0) { Serial.println(F("(empty)")); return; }
        for (uint8_t i = 0; i < store.count(); i++) {
            const CanMonitor::CanEntry* e = store.getAt(i);
            if (!e) continue;
            Serial.print(F("ID=0x")); Serial.print(e->id, HEX);
            Serial.print(F(" data="));
            for (uint8_t b = 0; b < e->length; b++) {
                if (e->data[b] < 0x10) Serial.print('0');
                Serial.print(e->data[b], HEX);
                Serial.print(' ');
            }
            Serial.println();
        }

    } else if (strcmp_P(cmd, PSTR("RESET")) == 0) {
        Serial.println(F("Resetting..."));
        Serial.flush();
        void (*reset)() = nullptr;
        reset();

    } else if (strncmp_P(cmd, PSTR("DOWNLOAD:"), 9) == 0) {
        if (!sdReady) { Serial.println(F("ERR: SD not available")); return; }
        char folder[8];
        sprintf(folder, "%04X", deviceId);
        char path[24];
        sprintf(path, "%s/%s_%s.csv", folder, folder, cmd + 9);
        File f = SD.open(path);
        if (!f) { Serial.print(F("ERR: not found: ")); Serial.println(path); return; }
        Serial.print(F("BEGIN:")); Serial.println(path);
        while (f.available()) Serial.write(f.read());
        f.close();
        Serial.println(F("END"));

    } else if (strcmp_P(cmd, PSTR("SYSRESET")) == 0) {
        sendBusReset();
        Serial.println(F("SYSRESET broadcast."));

    } else if (strncmp_P(cmd, PSTR("SET:"), 4) == 0) {
        // SET:<TYPE>:<PARAM>:<VALUE> — mutates cmd in place via strtok-style splitting
        char* p  = cmd + 4;
        char* c1 = strchr(p, ':');
        if (!c1) { Serial.println(F("ERR: SET:<TYPE>:<PARAM>:<VALUE>")); return; }
        *c1 = '\0';
        char* c2 = strchr(c1 + 1, ':');
        if (!c2) { Serial.println(F("ERR: SET:<TYPE>:<PARAM>:<VALUE>")); return; }
        *c2 = '\0';
        char*    typeStr  = p;
        char*    paramStr = c1 + 1;
        uint16_t value    = (uint16_t)atoi(c2 + 1);
        uint8_t  targetType, paramId;
        if (!lookupConfig(typeStr, paramStr, targetType, paramId)) {
            Serial.println(F("ERR: unknown type/param"));
            return;
        }
        sendConfig(targetType, paramId, value);
        applyLocalConfig(targetType, paramId, value);
        Serial.print(F("CONFIG sent: ")); Serial.print(typeStr);
        Serial.print(':'); Serial.print(paramStr);
        Serial.print('='); Serial.println(value);

    } else {
        Serial.println(F("Commands: TIME:<unix>  DEVID:<hex4>  AIRCRAFT:<hex4>  SPEED:<kbps>  CLEAR  LIST  RESET  DOWNLOAD:<date>  SYSRESET  SET:<TYPE>:<PARAM>:<VALUE>"));
    }
}


void sendPing() {
    const char* canState;
    if      (busError)   canState = "ERR";
    else if (busActive)  canState = "ACT";
    else if (store.count() > 0) canState = "TMO";
    else                 canState = "WAIT";

    char ts[20];
    rtcClock.getTimestamp(ts);

    Serial.print(F("PING can="));
    Serial.print(canState);
    Serial.print(F(" ids="));
    Serial.print(store.count());
    Serial.print(F(" sd="));
    Serial.print(sdReady ? F("OK") : F("FAIL"));
    Serial.print(F(" rtc="));
    Serial.println(rtcClock.isSet() ? ts : "none");
}

bool runCanStartup(CanBusInterface::Mode finalMode) {
    if (can.begin(SPEED_TABLE[canSpeedIndex], CanBusInterface::MODE_LOOPBACK) != CanBusInterface::OK) {
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
    initMsg.data[1]  = 0x00;
    initMsg.data[2]  = 0x00;
    memset(initMsg.data + 3, 0, 5);
    can.sendMessage(initMsg);

    // Re-init to clear any Bus-Off state from the unanswered SYSTEM_INIT broadcast.
    if (can.begin(SPEED_TABLE[canSpeedIndex], finalMode) != CanBusInterface::OK) {
        Serial.println(F("CAN init FAILED"));
        return false;
    }

    return true;
}

void setup() {
    Serial.begin(115200);
    Wire.begin();
    SPI.begin();

    loadConfig();

    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Canbus Monitor"));
    {
        char verBuf[17];
        snprintf(verBuf, sizeof(verBuf), "v%u.%u.%u", FW_MAJOR, FW_MINOR, FW_BUILD);
        lcd.setCursor(0, 1);
        lcd.print(verBuf);
    }

    rotary.begin();

    bool rtcFound = rtcClock.begin();
    if (!rtcFound) {
        Serial.println(F("RTC not found — using relative timestamps"));
    } else if (!rtcClock.isSet()) {
        Serial.println(F("RTC found but not set — send TIME:<unix>"));
    } else {
        Serial.println(F("RTC OK"));
    }

    sdLogger = new CanMonitor::SdLogger(PIN_SD_CS, rtcClock, deviceId, aircraftId);
    sdReady  = sdLogger->begin();
    if (!sdReady) {
        Serial.println(F("SD init FAILED"));
    } else {
        Serial.println(F("SD OK"));
    }

    Serial.println(F("CANBUS_MONITOR_STARTED"));
    Serial.print(F("Firmware: v"));
    Serial.print(FW_MAJOR); Serial.print('.'); Serial.print(FW_MINOR); Serial.print('.'); Serial.println(FW_BUILD);
    bool canOk = runCanStartup(CanBusInterface::MODE_NORMAL);

    if (canOk) {
        busError = false;
        Serial.print(F("CAN OK, speed: "));
        static const char* labels[] = { "125", "250", "500" };
        Serial.print(labels[canSpeedIndex]);
        Serial.println(F(" kbps"));
    } else {
        busError = true;
        Serial.println(F("CAN FAILED"));
    }

    Serial.print(F("Device ID:   0x")); Serial.println(deviceId,   HEX);
    Serial.print(F("Aircraft ID: 0x")); Serial.println(aircraftId, HEX);

    unsigned long now = millis();
    lastDisplayMs = now;
    lastPingMs    = now;  // send first ping immediately, then every PING_INTERVAL_MS
    // CGRAM must be written after all I2C peripherals are initialised to avoid
    // leaving the bus in an unexpected state during RTC/SD startup.
    uint8_t backslashGlyph[8] = {0x10, 0x08, 0x04, 0x02, 0x01, 0x00, 0x00, 0x00};
    lcd.createChar(0, backslashGlyph);
    uint8_t degreeGlyph[8]   = {0x06, 0x09, 0x09, 0x06, 0x00, 0x00, 0x00, 0x00};
    lcd.createChar(1, degreeGlyph);
    lcd.setCursor(0, 0);  // Return LCD to DDRAM mode after createChar
    updateDisplay();
    sendPing();
}

void loop() {
    rotary.poll();

    int8_t step = rotary.getStep();
    uint8_t total = totalItems();
    if (step != 0 && total > 0) {
        selectedIndex += step;
        if (selectedIndex < 0)                    selectedIndex = (int8_t)(total - 1);
        if (selectedIndex >= (int8_t)total)       selectedIndex = 0;
    }

    // Short press: toggle backlight.  Hold ≥ 5 s: software reset.
    {
        static unsigned long keyPressedAt = 0;
        static bool          keyHandled   = false;

        if (rotary.wasButtonPressed()) {
            keyPressedAt = millis();
            keyHandled   = false;
        }
        if (keyPressedAt > 0) {
            bool stillHeld = rotary.isButtonDown();
            if (!keyHandled && (millis() - keyPressedAt >= 5000)) {
                keyHandled = true;
                Serial.println(F("KEY held 5s — resetting..."));
                Serial.flush();
                void (*reset)() = nullptr;
                reset();
            }
            if (!stillHeld) {
                if (!keyHandled) {
                    lcdBacklightOn = !lcdBacklightOn;
                    if (lcdBacklightOn) lcd.backlight(); else lcd.noBacklight();
                }
                keyPressedAt = 0;
                keyHandled   = false;
            }
        }
    }

    if (!busError && can.messageAvailable()) {
        CanBusInterface::Message msg;
        if (can.receiveMessage(msg) == CanBusInterface::OK) {
            if (msg.id < CAN_ID_CONFIG) {  // skip system/command IDs (≥ 0x7E0)
                store.update(msg);
            }
            lastMessageMs = millis();
            busActive = true;

            Serial.print(F("CAN:0x"));
            Serial.print(msg.id, HEX);
            Serial.print(':');
            for (uint8_t i = 0; i < msg.length; i++) {
                if (msg.data[i] < 0x10) Serial.print('0');
                Serial.print(msg.data[i], HEX);
            }
            Serial.println();

            if (sdLogger) sdLogger->log(msg);
        }
    }

    if (!busError && busActive && (millis() - lastMessageMs > BUS_TIMEOUT_MS)) {
        busActive = false;
    }

    processSerial();

    unsigned long now = millis();
    if (now - lastDisplayMs >= DISPLAY_REFRESH_MS) {
        lastDisplayMs = now;
        updateDisplay();
    }

    if (now - lastPingMs >= PING_INTERVAL_MS) {
        lastPingMs = now;
        sendPing();
    }
}
