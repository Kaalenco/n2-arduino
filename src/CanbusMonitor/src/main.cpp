#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include <CanBusMCP2515.h>
#include <CanMessageStore.h>
#include <RotaryControl.h>
#include <PinsMap.h>
#include <CanMonitorMemoryMap.h>

static const unsigned long DISPLAY_REFRESH_MS = 250;
static const unsigned long BUS_TIMEOUT_MS     = 3000;

static const CanBusInterface::Speed SPEED_TABLE[] = {
    CanBusInterface::SPEED_125KBPS,
    CanBusInterface::SPEED_250KBPS,
    CanBusInterface::SPEED_500KBPS,
};

enum DisplayMode : uint8_t {
    DISPLAY_RAW_HEX = 0,
    DISPLAY_UINT16  = 1,
    DISPLAY_MODES_COUNT = 2,
};

#ifdef BUILD_SIMULATOR
struct SimEntry {
    uint16_t canId;
    uint16_t intervalMs;
    uint16_t value;
};

static const SimEntry SIM_TABLE[] PROGMEM = {
    { 0x0C0, 1000, 2400 },
    { 0x0C1, 1000, 2390 },
    { 0x101, 2000, 1013 },
    { 0x102,  500,  980 },
};
static const uint8_t SIM_TABLE_SIZE = sizeof(SIM_TABLE) / sizeof(SIM_TABLE[0]);
static unsigned long simLastSentMs[SIM_TABLE_SIZE] = {};
#endif

LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);
CanBusMCP2515 can;
CanMonitor::CanMessageStore store;
CanMonitor::RotaryControl rotary(PIN_ENCODER_CLK, PIN_ENCODER_DT, PIN_ENCODER_BTN);

uint8_t     canSpeedIndex;
int8_t      selectedIndex  = 0;
DisplayMode displayMode    = DISPLAY_RAW_HEX;
bool        busActive      = false;
bool        busError       = false;
unsigned long lastMessageMs = 0;
unsigned long lastDisplayMs = 0;


void loadConfig() {
    if (EEPROM.read(EEPROM_CM_MAGIC) != EEPROM_CM_MAGIC_VALUE) {
        EEPROM.update(EEPROM_CM_MAGIC,     EEPROM_CM_MAGIC_VALUE);
        EEPROM.update(EEPROM_CM_CAN_SPEED, CM_DEFAULT_CAN_SPEED);
    }
    canSpeedIndex = EEPROM.read(EEPROM_CM_CAN_SPEED);
    if (canSpeedIndex > 2) canSpeedIndex = CM_DEFAULT_CAN_SPEED;
}

void lcdRow(uint8_t row, const char* text) {
    char buf[LCD_COLS + 1];
    snprintf(buf, sizeof(buf), "%-16s", text);
    lcd.setCursor(0, row);
    lcd.print(buf);
}

void updateDisplay() {
    char line[LCD_COLS + 1];

    if (busError) {
        lcdRow(0, "BUS ERROR       ");
    } else if (busActive) {
        snprintf(line, sizeof(line), "ACTIVE  %2u IDs  ", store.count());
        lcdRow(0, line);
    } else if (store.count() > 0) {
        snprintf(line, sizeof(line), "TIMEOUT %2u IDs  ", store.count());
        lcdRow(0, line);
    } else {
        lcdRow(0, "WAITING...      ");
    }

    const CanMonitor::CanEntry* entry = store.getAt((uint8_t)selectedIndex);
    if (entry == nullptr) {
        lcdRow(1, "No data         ");
    } else if (displayMode == DISPLAY_RAW_HEX) {
        snprintf(line, sizeof(line), "%03lX %02X%02X %02X%02X   ",
                 entry->id,
                 entry->data[0], entry->data[1],
                 entry->data[2], entry->data[3]);
        lcdRow(1, line);
    } else {
        uint16_t w0 = entry->data[0] | ((uint16_t)entry->data[1] << 8);
        uint16_t w1 = entry->data[2] | ((uint16_t)entry->data[3] << 8);
        snprintf(line, sizeof(line), "%03lX %5u %5u  ", entry->id, w0, w1);
        lcdRow(1, line);
    }
}

void processSerial() {
    if (!Serial.available()) return;
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd.startsWith(F("SPEED:"))) {
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
        Serial.print(F("CAN speed set to "));
        Serial.print(speed);
        Serial.println(F(" kbps. Restart to apply."));
    } else if (cmd == F("CLEAR")) {
        store.clear();
        busActive = false;
        selectedIndex = 0;
        Serial.println(F("Store cleared."));
    } else if (cmd == F("LIST")) {
        if (store.count() == 0) {
            Serial.println(F("(empty)"));
            return;
        }
        for (uint8_t i = 0; i < store.count(); i++) {
            const CanMonitor::CanEntry* e = store.getAt(i);
            if (!e) continue;
            Serial.print(F("ID=0x"));
            Serial.print(e->id, HEX);
            Serial.print(F(" data="));
            for (uint8_t b = 0; b < e->length; b++) {
                if (e->data[b] < 0x10) Serial.print('0');
                Serial.print(e->data[b], HEX);
                Serial.print(' ');
            }
            Serial.println();
        }
    } else {
        Serial.println(F("Commands: SPEED:<kbps>  CLEAR  LIST"));
    }
}

#ifdef BUILD_SIMULATOR
void runSimulator() {
    unsigned long now = millis();
    for (uint8_t i = 0; i < SIM_TABLE_SIZE; i++) {
        SimEntry entry;
        memcpy_P(&entry, &SIM_TABLE[i], sizeof(SimEntry));
        if (now - simLastSentMs[i] < entry.intervalMs) continue;
        simLastSentMs[i] = now;

        CanBusInterface::Message msg;
        msg.id       = entry.canId;
        msg.length   = 4;
        msg.extended = false;
        msg.rtr      = false;
        msg.data[0]  = entry.value & 0xFF;
        msg.data[1]  = (entry.value >> 8) & 0xFF;
        msg.data[2]  = 0;
        msg.data[3]  = 0;
        memset(msg.data + 4, 0, 4);

        if (can.sendMessage(msg) == CanBusInterface::OK) {
            store.update(msg);
            lastMessageMs = now;
            busActive = true;
        }
    }
}
#endif

void setup() {
    Serial.begin(9600);
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

#ifdef BUILD_SIMULATOR
    Serial.println(F("CANBUS_MONITOR_SIMULATOR_STARTED"));
    CanBusInterface::Result r = can.begin(SPEED_TABLE[canSpeedIndex], CanBusInterface::MODE_NORMAL);
#else
    Serial.println(F("CANBUS_MONITOR_STARTED"));
    CanBusInterface::Result r = can.begin(SPEED_TABLE[canSpeedIndex], CanBusInterface::MODE_LISTEN_ONLY);
#endif

    if (r == CanBusInterface::OK) {
        busError = false;
        Serial.print(F("CAN OK, speed: "));
        static const char* labels[] = { "125", "250", "500" };
        Serial.print(labels[canSpeedIndex]);
        Serial.println(F(" kbps"));
    } else {
        busError = true;
        Serial.println(F("CAN FAILED"));
    }

    lastDisplayMs = millis();
    updateDisplay();
}

void loop() {
    rotary.poll();

    int8_t step = rotary.getStep();
    if (step != 0 && store.count() > 0) {
        selectedIndex += step;
        if (selectedIndex < 0) selectedIndex = (int8_t)(store.count() - 1);
        if (selectedIndex >= (int8_t)store.count()) selectedIndex = 0;
    }

    if (rotary.wasButtonPressed()) {
        displayMode = (DisplayMode)((displayMode + 1) % DISPLAY_MODES_COUNT);
    }

#ifdef BUILD_SIMULATOR
    if (!busError) {
        runSimulator();
    }
#else
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
        }
    }
#endif

    if (!busError && busActive && (millis() - lastMessageMs > BUS_TIMEOUT_MS)) {
        busActive = false;
    }

    processSerial();

    unsigned long now = millis();
    if (now - lastDisplayMs >= DISPLAY_REFRESH_MS) {
        lastDisplayMs = now;
        updateDisplay();
    }
}
