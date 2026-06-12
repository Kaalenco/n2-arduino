#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <mcp_can.h>
#include <Adafruit_BMP085.h>
#include "version.h"

static const uint8_t PIN_CAN_CS  = 10;
static const uint8_t PIN_T1_CS   = 2;
static const uint8_t PIN_BUZZER  = A0;

static const uint32_t CAN_ID_SYSTEM_INIT = 0x7F0;
static const uint32_t CAN_ID_SYSRESET   = 0x7EF;
static const uint8_t  SYSTEM_TYPE_BFI   = 0x03;

// CanFIX parameter IDs
static const uint16_t CAN_ID_ALT     = 388;   // 0x184 Indicated Altitude, DINT, ft
static const uint16_t CAN_ID_OAT     = 1030;  // 0x406 Total Air Temperature, INT, x0.01 C
static const uint16_t CAN_ID_IAT     = 1031;  // 0x407 Static Air Temperature, INT, x0.01 C
static const uint16_t CAN_ID_ALT_SET = 400;   // 0x190 Altimeter Setting, UINT, x0.001 inHg (incoming)

static const unsigned long SENSOR_INTERVAL_MS = 1000;
static const float INHG_TO_PA     = 3386.389f;
static const float DEFAULT_QNH_PA = 101325.0f;

MCP_CAN         can(PIN_CAN_CS);
Adafruit_BMP085 bmp;

bool          canReady   = false;
bool          bmpReady   = false;
float         qnhPa      = DEFAULT_QNH_PA;
unsigned long lastSensorMs = 0;

static char    serialBuf[16];
static uint8_t serialLen = 0;

// Returns temperature in degrees C, or NAN on open-thermocouple fault.
static float readMAX6675() {
    SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
    digitalWrite(PIN_T1_CS, LOW);
    delayMicroseconds(2);
    uint16_t raw = ((uint16_t)SPI.transfer(0x00) << 8) | SPI.transfer(0x00);
    digitalWrite(PIN_T1_CS, HIGH);
    SPI.endTransaction();
    if (raw & 0x04) return NAN;  // open-thermocouple fault bit
    return (float)(raw >> 3) * 0.25f;
}

static bool sendFrame16(uint16_t id, int16_t value) {
    uint8_t data[2] = { (uint8_t)(value & 0xFF), (uint8_t)((value >> 8) & 0xFF) };
    return can.sendMsgBuf((uint32_t)id, 0, 2, data) == CAN_OK;
}

static bool sendFrame32(uint16_t id, int32_t value) {
    uint8_t data[4] = {
        (uint8_t)(value & 0xFF),
        (uint8_t)((value >> 8) & 0xFF),
        (uint8_t)((value >> 16) & 0xFF),
        (uint8_t)((value >> 24) & 0xFF)
    };
    return can.sendMsgBuf((uint32_t)id, 0, 4, data) == CAN_OK;
}

static void applyQnh(float pa) {
    qnhPa = pa;
    Serial.print(F("QNH: "));
    Serial.print(pa / 100.0f, 2);
    Serial.println(F(" hPa"));
}

static void parseSerial(const char* line) {
    if (strncmp(line, "QNH=", 4) == 0) {
        float hpa = atof(line + 4);
        if (hpa > 800.0f && hpa < 1100.0f)
            applyQnh(hpa * 100.0f);
        else
            Serial.println(F("ERR: QNH out of range (800-1100 hPa)"));
    } else {
        Serial.println(F("Commands: QNH=<hPa>"));
    }
}

void setup() {
    Serial.begin(115200);
    Wire.begin();
    SPI.begin();

    pinMode(PIN_T1_CS, OUTPUT);
    digitalWrite(PIN_T1_CS, HIGH);
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);

    if (can.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
        can.setMode(MCP_NORMAL);
        canReady = true;
        uint8_t initData[3] = { SYSTEM_TYPE_BFI, 0x00, 0x00 };
        can.sendMsgBuf(CAN_ID_SYSTEM_INIT, 0, 3, initData);
        Serial.println(F("CAN OK"));
    } else {
        Serial.println(F("CAN FAILED"));
    }

    bmpReady = bmp.begin();
    Serial.println(bmpReady ? F("BMP085 OK") : F("BMP085 FAILED"));

    float t = readMAX6675();
    Serial.println(isnan(t) ? F("MAX6675 FAILED") : F("MAX6675 OK"));

    Serial.print(F("BFI v"));
    Serial.print(FW_MAJOR); Serial.print('.'); Serial.print(FW_MINOR); Serial.print('.'); Serial.println(FW_BUILD);
    Serial.println(F("QNH=<hPa> to set altimeter setting"));
    lastSensorMs = millis();
}

static void checkIncoming() {
    if (can.checkReceive() != CAN_MSGAVAIL) return;
    unsigned long rxId;
    uint8_t len;
    uint8_t data[8] = {};
    can.readMsgBuf(&rxId, &len, data);

    if (rxId == CAN_ID_SYSRESET) {
        Serial.println(F("SYSRESET"));
        Serial.flush();
        void (*reset)() = nullptr;
        reset();
    } else if (rxId == CAN_ID_ALT_SET && len >= 2) {
        uint16_t raw = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
        applyQnh(raw * 0.001f * INHG_TO_PA);
    }
}

static void checkSerial() {
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\n' || c == '\r') {
            if (serialLen > 0) {
                serialBuf[serialLen] = '\0';
                parseSerial(serialBuf);
                serialLen = 0;
            }
        } else if (serialLen < (uint8_t)(sizeof(serialBuf) - 1)) {
            serialBuf[serialLen++] = c;
        }
    }
}

void loop() {
    if (canReady) checkIncoming();
    checkSerial();

    unsigned long now = millis();
    if (now - lastSensorMs < SENSOR_INTERVAL_MS) return;
    lastSensorMs = now;

    float tempC = readMAX6675();
    if (!isnan(tempC)) {
        int16_t oat = (int16_t)(tempC * 100.0f);
        if (canReady) sendFrame16(CAN_ID_OAT, oat);
        Serial.print(F("OAT: ")); Serial.print(tempC, 1); Serial.println(F(" C"));
    } else {
        Serial.println(F("OAT: open fault"));
    }

    if (bmpReady) {
        float iatC = bmp.readTemperature();
        int16_t iat = (int16_t)(iatC * 100.0f);
        if (canReady) sendFrame16(CAN_ID_IAT, iat);
        Serial.print(F("IAT: ")); Serial.print(iatC, 1); Serial.println(F(" C"));

        float altM = bmp.readAltitude(qnhPa);
        int32_t altFt = (int32_t)(altM * 3.28084f);
        if (canReady) sendFrame32(CAN_ID_ALT, altFt);
        Serial.print(F("Alt: ")); Serial.print(altFt); Serial.println(F(" ft"));
    }
}
