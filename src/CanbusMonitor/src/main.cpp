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

enum DisplayMode : uint8_t {
    DISPLAY_RAW_HEX = 0,
    DISPLAY_UINT16  = 1,
    DISPLAY_MODES_COUNT = 2,
};


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
DisplayMode displayMode    = DISPLAY_RAW_HEX;
bool        busActive      = false;
bool        busError       = false;
bool        sdReady        = false;
unsigned long lastMessageMs = 0;
unsigned long lastDisplayMs = 0;
unsigned long lastPingMs    = 0;


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
    static const char spinnerChars[] = {'|', '/', '-', '\\'};

    char line[LCD_COLS + 1];

    if (busError) {
        lcdRow(0, "BUS ERROR");
    } else if (busActive) {
        snprintf(line, sizeof(line), "ACT %2u IDs %s",
                 store.count(), sdReady ? "SD" : "  ");
        lcdRow(0, line);
    } else if (store.count() > 0) {
        snprintf(line, sizeof(line), "TMO %2u IDs %s",
                 store.count(), sdReady ? "SD" : "  ");
        lcdRow(0, line);
    } else {
        lcdRow(0, sdReady ? "WAITING... SD" : "WAITING...");
    }

    lcd.setCursor(15, 0);
    lcd.write(spinnerChars[spinnerIdx]);
    spinnerIdx = (spinnerIdx + 1) % 4;

    const CanMonitor::CanEntry* entry = store.getAt((uint8_t)selectedIndex);
    if (entry == nullptr) {
        lcdRow(1, "No data");
    } else if (displayMode == DISPLAY_RAW_HEX) {
        snprintf(line, sizeof(line), "%03lX %02X%02X %02X%02X",
                 entry->id,
                 entry->data[0], entry->data[1],
                 entry->data[2], entry->data[3]);
        lcdRow(1, line);
    } else {
        uint16_t w0 = entry->data[0] | ((uint16_t)entry->data[1] << 8);
        uint16_t w1 = entry->data[2] | ((uint16_t)entry->data[3] << 8);
        snprintf(line, sizeof(line), "%03lX %5u %5u", entry->id, w0, w1);
        lcdRow(1, line);
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

// Returns true and fills targetType/paramId if type/param strings are known.
// Param mnemonics are 3 characters — see docs/can-param-mnemonics.md.
static bool lookupConfig(const String& type, const String& param,
                         uint8_t& targetType, uint8_t& paramId) {
    if (type == F("RPM")) {
        targetType = 0x01;
        if (param == F("GRL")) { paramId = 0x01; return true; }  // green arc low
        if (param == F("GRH")) { paramId = 0x02; return true; }  // green arc high
        if (param == F("RED")) { paramId = 0x03; return true; }  // red line
    } else if (type == F("ALT")) {
        targetType = 0x02;
        if (param == F("QNH")) { paramId = 0x01; return true; }  // altimeter setting
    } else if (type == F("EGT")) {
        targetType = 0x03;
        if (param == F("CAL")) { paramId = 0x01; return true; }  // caution low
        if (param == F("CAH")) { paramId = 0x02; return true; }  // caution high
    } else if (type == F("CHT")) {
        targetType = 0x04;
        if (param == F("CAL")) { paramId = 0x01; return true; }  // caution low
        if (param == F("CAH")) { paramId = 0x02; return true; }  // caution high
    }
    return false;
}

// Parse a 4-digit hex string to uint16.  Returns false if not valid hex.
static bool parseHex16(const String& s, uint16_t& out) {
    if (s.length() != 4) return false;
    char* end;
    unsigned long v = strtoul(s.c_str(), &end, 16);
    if (*end != '\0') return false;
    out = (uint16_t)v;
    return true;
}

void processSerial() {
    if (!Serial.available()) return;
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd.startsWith(F("TIME:"))) {
        // TIME:<unix_timestamp>  e.g. TIME:1749600000
        uint32_t unixTime = (uint32_t)cmd.substring(5).toInt();
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

    } else if (cmd.startsWith(F("DEVID:"))) {
        // DEVID:<hex4>  e.g. DEVID:0042
        uint16_t id;
        if (!parseHex16(cmd.substring(6), id) || id == 0) {
            Serial.println(F("ERR: DEVID expects 4-digit hex, non-zero"));
            return;
        }
        EEPROM.put(EEPROM_CM_DEVICE_ID, id);
        deviceId = id;
        if (sdLogger) sdLogger->setDeviceId(id);
        Serial.print(F("Device ID set to 0x")); Serial.println(id, HEX);

    } else if (cmd.startsWith(F("AIRCRAFT:"))) {
        // AIRCRAFT:<hex4>  e.g. AIRCRAFT:PH42
        uint16_t id;
        if (!parseHex16(cmd.substring(9), id)) {
            Serial.println(F("ERR: AIRCRAFT expects 4-digit hex"));
            return;
        }
        EEPROM.put(EEPROM_CM_AIRCRAFT_ID, id);
        aircraftId = id;
        if (sdLogger) sdLogger->setAircraftId(id);
        Serial.print(F("Aircraft ID set to 0x")); Serial.println(id, HEX);

    } else if (cmd.startsWith(F("SPEED:"))) {
        int speed = cmd.substring(6).toInt();
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

    } else if (cmd == F("CLEAR")) {
        store.clear();
        busActive = false;
        selectedIndex = 0;
        Serial.println(F("Store cleared."));

    } else if (cmd == F("LIST")) {
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

    } else if (cmd == F("RESET")) {
        Serial.println(F("Resetting..."));
        Serial.flush();
        void (*reset)() = nullptr;
        reset();

    } else if (cmd.startsWith(F("DOWNLOAD:"))) {
        // DOWNLOAD:<YYYYMMDD> — dump named CSV file to serial
        String dateStr = cmd.substring(9);
        dateStr.trim();
        if (!sdReady) { Serial.println(F("ERR: SD not available")); return; }
        char folder[8];
        sprintf(folder, "%04X", deviceId);
        char path[24];
        sprintf(path, "%s/%s_%s.csv", folder, folder, dateStr.c_str());
        File f = SD.open(path);
        if (!f) { Serial.print(F("ERR: not found: ")); Serial.println(path); return; }
        Serial.print(F("BEGIN:")); Serial.println(path);
        while (f.available()) {
            Serial.write(f.read());
        }
        f.close();
        Serial.println(F("END"));

    } else if (cmd == F("SYSRESET")) {
        sendBusReset();
        Serial.println(F("SYSRESET broadcast."));

    } else if (cmd.startsWith(F("SET:"))) {
        // SET:<TYPE>:<PARAM>:<VALUE>  e.g. SET:RPM:GRL:700
        String rest = cmd.substring(4);
        int c1 = rest.indexOf(':');
        if (c1 < 0) { Serial.println(F("ERR: SET:<TYPE>:<PARAM>:<VALUE>")); return; }
        String typeStr  = rest.substring(0, c1);
        String rest2    = rest.substring(c1 + 1);
        int c2          = rest2.indexOf(':');
        if (c2 < 0) { Serial.println(F("ERR: SET:<TYPE>:<PARAM>:<VALUE>")); return; }
        String paramStr = rest2.substring(0, c2);
        uint16_t value  = (uint16_t)rest2.substring(c2 + 1).toInt();
        uint8_t targetType, paramId;
        if (!lookupConfig(typeStr, paramStr, targetType, paramId)) {
            Serial.println(F("ERR: unknown type/param"));
            return;
        }
        sendConfig(targetType, paramId, value);
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
    lcd.setCursor(0, 1);
    lcd.print(F("Initializing..."));

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

    lastDisplayMs = millis();
    updateDisplay();
}

void loop() {
    rotary.poll();

    int8_t step = rotary.getStep();
    if (step != 0 && store.count() > 0) {
        selectedIndex += step;
        if (selectedIndex < 0)                        selectedIndex = (int8_t)(store.count() - 1);
        if (selectedIndex >= (int8_t)store.count())   selectedIndex = 0;
    }

    if (rotary.wasButtonPressed()) {
        displayMode = (DisplayMode)((displayMode + 1) % DISPLAY_MODES_COUNT);
    }

    if (!busError && can.messageAvailable()) {
        CanBusInterface::Message msg;
        if (can.receiveMessage(msg) == CanBusInterface::OK) {
            store.update(msg);
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
