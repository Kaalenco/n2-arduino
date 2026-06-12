#include <Arduino.h>
#include <SPI.h>
#include <mcp_can.h>

// Generic MCP2515 breakout board: 8 MHz crystal, CS on D10.
static const uint8_t  PIN_CAN_CS        = 10;
static const uint32_t CAN_ID_SYSTEM_INIT = 0x7F0;
static const uint8_t  SYSTEM_TYPE_MOCK   = 0x10;  // CanMock

struct MockEntry {
    uint16_t canId;
    uint16_t intervalMs;
    uint16_t value;
};

// Fixed mock frames transmitted on schedule.
// value is sent as uint16 little-endian in bytes 0-1; bytes 2-3 are zero.
static const MockEntry MOCK_TABLE[] PROGMEM = {
    { 0x0C0, 1000, 2400 },   // RPM
    { 0x0C1, 1000, 2390 },   // RPM (secondary)
    { 0x101, 2000, 1013 },   // Baro pressure (mbar)
    { 0x102,  500,  980 },   // Altitude pressure (mbar)
};
static const uint8_t MOCK_TABLE_SIZE = sizeof(MOCK_TABLE) / sizeof(MOCK_TABLE[0]);
static unsigned long lastSentMs[MOCK_TABLE_SIZE] = {};

MCP_CAN can(PIN_CAN_CS);
bool canReady = false;

bool sendFrame(uint16_t id, uint16_t value) {
    uint8_t data[4] = {
        (uint8_t)(value & 0xFF),
        (uint8_t)(value >> 8),
        0, 0
    };
    return can.sendMsgBuf((uint32_t)id, 0, 4, data) == CAN_OK;
}

void setup() {
    Serial.begin(115200);

    if (can.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) != CAN_OK) {
        Serial.println(F("CAN init FAILED"));
        return;
    }
    can.setMode(MCP_NORMAL);
    canReady = true;

    uint8_t initData[3] = { SYSTEM_TYPE_MOCK, 0x00, 0x00 };
    can.sendMsgBuf(CAN_ID_SYSTEM_INIT, 0, 3, initData);

    Serial.println(F("CAN_MOCK_STARTED"));
}

void loop() {
    if (!canReady) return;

    unsigned long now = millis();
    for (uint8_t i = 0; i < MOCK_TABLE_SIZE; i++) {
        MockEntry entry;
        memcpy_P(&entry, &MOCK_TABLE[i], sizeof(MockEntry));
        if (now - lastSentMs[i] < entry.intervalMs) continue;
        lastSentMs[i] = now;

        if (sendFrame(entry.canId, entry.value)) {
            Serial.print(F("TX 0x"));
            Serial.print(entry.canId, HEX);
            Serial.print(F(" = "));
            Serial.println(entry.value);
        } else {
            Serial.print(F("TX FAILED 0x"));
            Serial.println(entry.canId, HEX);
        }
    }
}
