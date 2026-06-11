#ifndef ENGINE_DATA_1_DECODER_H
#define ENGINE_DATA_1_DECODER_H

#include <Arduino.h>

namespace CanbusReceiver {

/**
 * Alert flags for ENGINE_DATA_1 message (12 bits available)
 */
enum EngineAlerts : uint16_t {
    ALERT_NONE = 0x000,
    ALERT_EGT_HIGH = 0x001,
    ALERT_CHT_HIGH = 0x002,
    ALERT_OIL_TEMP_HIGH = 0x004,
    ALERT_OIL_PRESSURE_LOW = 0x008,
    ALERT_ENGINE_OVERHEAT = 0x010,
    ALERT_SENSOR_FAILURE = 0x020,
    // Bits 7-8 reserved for future use
};

/**
 * Data structure for ENGINE_DATA_1 message
 */
struct EngineData1 {
    int16_t egt1Celsius;
    bool egt1OverTemp;

    int16_t cht1Celsius;
    bool cht1OverTemp;

    int16_t oilTempCelsius;
    bool oilTempOverTemp;

    int16_t engineAmbientCelsius;
    bool engineAmbientOverTemp;

    int16_t oilPressureX10;  // Pressure in bar * 10 (0-120)
    bool oilPressureUnder;

    uint16_t rpm;  // Engine RPM (0-5000)

    uint8_t alerts;  // 12-bit alert flags
};

/**
 * Decoder for ENGINE_DATA_1 CAN message
 *
 * Packs engine sensor data into 8-byte CAN frame:
 * - EGT1: 8 + 1 bits (8 bits value 0-256 + 1 bit over-temp) with an offset of 250°C, step 2°C, range 250°C to +762°C. Values above 762°C are clamped to 762°C. Values below 250°C are clamped to 250°C.
 * - CHT1: 8 + 1 bits (8 bits value 0-256 + 1 bit over-temp) with an offset of 50°C, step 1°C, range 50°C to +306°C. Values above 306°C are clamped to 306°C. Values below 50°C are clamped to 50°C.
 * - OIL_TEMP: 8 + 1 bits (8 bits value 0-256 + 1 bit over-temp) with an offset of 50°C, step 1°C, range 50°C to +306°C. Values above 306°C are clamped to 306°C. Values below 50°C are clamped to 50°C.
 * - ENGINE_AMBIENT: 8 + 1 bits (8 bits value 0-256 + 1 bit over-temp) with an offset of -50°C, step 3°C, range -50°C to +728°C. Values above 728°C are clamped to 728°C. Values below -50°C are clamped to -50°C.
 * - RPM : 8 bits (0-5000 RPM) - with a step of 20 RPM, range 0-5100 RPM (values above 5100 RPM are clamped)
 * - OIL_PRESSURE: 8 bits (8 bits value 0-256 + 1 bit under-pressure)
 * - ALERTS: 8 bits
 *
 * Bit layout in 8 bytes (64 bits):
 * Byte 0: EGT1[0-7]
 * Byte 1: CHT1[0-7]
 * Byte 2: OIL_TEMP[0-7]
 * Byte 3: ENGINE_AMBIENT[0-7]
 * Byte 4: OIL_PRESSURE[0-7]
 * Byte 5: RPM[0-7]
 * Byte 6: not used
 * Byte 7: ALERTS[0-7]
 */
class EngineData1Decoder {
public:

    /**
     * Unpack 8-byte buffer into engine data structure
     * @param buffer 8-byte input buffer
     * @param data Output data structure
     */
    static void unpack(const uint8_t* buffer, EngineData1& data) {
        // Reconstruct packed values
        uint8_t egt1 = buffer[0];
        uint8_t cht1 = buffer[1];
        uint8_t oilTemp = buffer[2];
        uint8_t engineAmbient = buffer[3];
        uint8_t oilPressure = buffer[4];
        uint8_t rpm = buffer[5];
        uint8_t warnings = buffer[6];
        uint8_t alerts = buffer[7];

        // Unpack temperatures
        unpackValue(egt1, data.egt1Celsius, data.egt1OverTemp,250,2, warnings, ALERT_EGT_HIGH);
        unpackValue(cht1, data.cht1Celsius, data.cht1OverTemp,50,1,warnings, ALERT_CHT_HIGH);
        unpackValue(oilTemp, data.oilTempCelsius, data.oilTempOverTemp,50,1,warnings, ALERT_OIL_TEMP_HIGH);
        unpackValue(engineAmbient, data.engineAmbientCelsius, data.engineAmbientOverTemp,-50,3,warnings, ALERT_ENGINE_OVERHEAT);

        // Unpack oil pressure
        unpackValue(oilPressure, data.oilPressureX10, data.oilPressureUnder, 0,1,warnings, ALERT_OIL_PRESSURE_LOW);

        // unpack RPM
        data.rpm = rpm * 20;

        // Alerts
        data.alerts = alerts;
    }

private:
    /**
     * Unpack a value (with alert checking)
     */
    static void unpackValue(uint16_t packed, int16_t& value, bool& warning, int16_t offset, uint8_t step, uint8_t alert, uint8_t alertBit) {
        value = packed & 0x03FF;  // Lower 10 bits
        warning = (alert & alertBit) != 0;  // Bit alertBit
        value = (value * step) + offset;
    }   
};

} // namespace CanbusReceiver

#endif // ENGINE_DATA_1_DECODER_H
