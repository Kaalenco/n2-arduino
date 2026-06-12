#include <Arduino.h>
#include <SPI.h>
#include <mcp_can.h>
#include "version.h"

// Generic MCP2515 breakout board: 8 MHz crystal, CS on D10.
static const uint8_t  PIN_CAN_CS         = 10;
static const uint32_t CAN_ID_SYSTEM_INIT = 0x7F0;
static const uint32_t CAN_ID_CONFIG      = 0x7E0;
static const uint32_t CAN_ID_SYSRESET    = 0x7EF;
static const uint8_t  SYSTEM_TYPE_MOCK   = 0x10;

// CanFIX parameter IDs — see docs/canfix/src/canfix.json
static const uint16_t CAN_ID_RPM = 512;   // 0x200 N1/Engine RPM, UINT, direct
static const uint16_t CAN_ID_EGT = 1282;  // 0x502 Exhaust Gas Temperature, UINT, x0.1 C
static const uint16_t CAN_ID_CHT = 1280;  // 0x500 Cylinder Head Temperature, UINT, x0.1 C

static const unsigned long RPM_SEND_MS  = 1000;
static const unsigned long TEMP_SEND_MS = 10000;

// --- RPM state machine -------------------------------------------
// Sequence: idle (700) 10 s → cruise (1700) 10 s → overspeed (2700) 2 s → repeat
enum RpmState : uint8_t { RPM_IDLE = 0, RPM_CRUISE = 1, RPM_OVERSPEED = 2 };
static const uint16_t      RPM_VALUE[]    = { 700,   1700,  2700  };
static const unsigned long RPM_DURATION[] = { 10000, 10000, 2000  };

static RpmState       rpmState    = RPM_IDLE;
static unsigned long  rpmStateMs  = 0;
static unsigned long  lastRpmMs   = 0;

// --- Temperature triangle waves ----------------------------------
// Values in 0.1 °C.  step cycles 0..199: 0-100 rising, 101-199 falling.
// EGT: 80.0–180.0 °C  (800–1800, 100 steps of 10)
// CHT: 70.0–120.0 °C  (700–1200, 100 steps of 5)
static uint8_t       egtStep   = 0;
static unsigned long lastEgtMs = 0;
static uint8_t       chtStep   = 0;
static unsigned long lastChtMs = 0;

MCP_CAN can(PIN_CAN_CS);
bool canReady = false;

// Triangle wave: step 0..199 maps to minVal at 0 and 200, maxVal at 100.
static uint16_t triangleVal(uint8_t step, uint16_t minVal, uint16_t maxVal) {
    uint8_t s = (step <= 100) ? step : (uint8_t)(200 - step);
    return minVal + (uint16_t)((uint32_t)s * (maxVal - minVal) / 100);
}

static bool sendFrame(uint16_t id, uint16_t value) {
    uint8_t data[4] = { (uint8_t)(value & 0xFF), (uint8_t)(value >> 8), 0, 0 };
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

    // SYSTEM_INIT identifies this node on the bus (best-effort; no ACK required).
    uint8_t initData[3] = { SYSTEM_TYPE_MOCK, 0x00, 0x00 };
    can.sendMsgBuf(CAN_ID_SYSTEM_INIT, 0, 3, initData);
    Serial.println(F("CAN_MOCK_STARTED"));
    Serial.print(F("Firmware: v"));
    Serial.print(FW_MAJOR); Serial.print('.'); Serial.print(FW_MINOR); Serial.print('.'); Serial.println(FW_BUILD);

    unsigned long now = millis();
    rpmStateMs = lastRpmMs = lastEgtMs = lastChtMs = now;
}

static void checkIncoming() {
    if (can.checkReceive() != CAN_MSGAVAIL) return;
    unsigned long rxId;
    uint8_t len;
    uint8_t data[8] = {};
    can.readMsgBuf(&rxId, &len, data);

    if (rxId == CAN_ID_SYSRESET) {
        Serial.println(F("SYSRESET received, resetting..."));
        Serial.flush();
        void (*reset)() = nullptr;
        reset();
    } else if (rxId == CAN_ID_CONFIG) {
        uint8_t  targetType = len > 0 ? data[0] : 0xFF;
        uint8_t  paramId    = len > 1 ? data[1] : 0;
        uint16_t value      = len > 3 ? (data[2] | ((uint16_t)data[3] << 8)) : 0;
        Serial.print(F("CONFIG type=0x")); Serial.print(targetType, HEX);
        Serial.print(F(" param=0x"));     Serial.print(paramId, HEX);
        Serial.print(F(" value="));       Serial.println(value);
    }
}

void loop() {
    if (!canReady) return;
    unsigned long now = millis();

    checkIncoming();

    // RPM — advance state machine then send on interval
    if (now - rpmStateMs >= RPM_DURATION[rpmState]) {
        rpmState   = (RpmState)((rpmState + 1) % 3);
        rpmStateMs = now;
        Serial.print(F("RPM state -> "));
        Serial.println(RPM_VALUE[rpmState]);
    }
    if (now - lastRpmMs >= RPM_SEND_MS) {
        lastRpmMs = now;
        sendFrame(CAN_ID_RPM, RPM_VALUE[rpmState]);
    }

    // EGT — triangle 80.0–180.0 °C, step every 10 s
    if (now - lastEgtMs >= TEMP_SEND_MS) {
        lastEgtMs = now;
        egtStep   = (egtStep + 1) % 200;
        uint16_t egt = triangleVal(egtStep, 800, 1800);
        sendFrame(CAN_ID_EGT, egt);
        Serial.print(F("EGT "));
        Serial.print(egt / 10); Serial.print('.'); Serial.println(egt % 10);
    }

    // CHT — triangle 70.0–120.0 °C, step every 10 s
    if (now - lastChtMs >= TEMP_SEND_MS) {
        lastChtMs = now;
        chtStep   = (chtStep + 1) % 200;
        uint16_t cht = triangleVal(chtStep, 700, 1200);
        sendFrame(CAN_ID_CHT, cht);
        Serial.print(F("CHT "));
        Serial.print(cht / 10); Serial.print('.'); Serial.println(cht % 10);
    }
}
