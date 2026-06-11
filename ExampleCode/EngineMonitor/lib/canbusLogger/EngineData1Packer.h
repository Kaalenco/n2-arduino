#ifndef ENGINE_DATA_1_PACKER_H
#define ENGINE_DATA_1_PACKER_H

#include <Arduino.h>

namespace CanbusLogging {

/**
 * Alert flags for ENGINE_DATA_1 message (8 bits available)
 */
enum EngineAlerts : uint8_t {
    ALERT_NONE = 0x00,
    ALERT_EGT_HIGH = 0x01,
    ALERT_CHT_HIGH = 0x02,
    ALERT_OIL_TEMP_HIGH = 0x04,
    ALERT_OIL_PRESSURE_LOW = 0x08,
    ALERT_ENGINE_OVERHEAT = 0x10,
    ALERT_SENSOR_FAILURE = 0x20,
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
 * Packer/unpacker for ENGINE_DATA_1 CAN message
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
class EngineData1Packer {
public:
    /**
     * Pack engine data into 8-byte buffer
     * @param data Source data structure
     * @param buffer 8-byte output buffer
     */
    static void pack(const EngineData1& data, uint8_t* buffer) {
        uint8_t warnings = 0;
        uint8_t egt1 = packValue(data.egt1Celsius, data.egt1OverTemp, 250, 2, warnings, ALERT_EGT_HIGH);
        uint8_t cht1 = packValue(data.cht1Celsius, data.cht1OverTemp, 50, 1, warnings, ALERT_CHT_HIGH);
        uint8_t oilTemp = packValue(data.oilTempCelsius, data.oilTempOverTemp, 50, 1, warnings, ALERT_OIL_TEMP_HIGH);
        uint8_t engineAmbient = packValue(data.engineAmbientCelsius, data.engineAmbientOverTemp, -50, 3, warnings, ALERT_ENGINE_OVERHEAT);
        uint8_t oilPressure = packValue(data.oilPressureX10, data.oilPressureUnder, 0, 1, warnings, ALERT_OIL_PRESSURE_LOW);
        uint8_t rpm = packValue(data.rpm, false, 0, 20);

        // Byte 0: EGT1
        buffer[0] = egt1;

        // Byte 1: CHT
        buffer[1] = cht1 ;

        // Byte 2: OIL_TEMP
        buffer[2] = oilTemp;

        // Byte 3: ENGINE_AMBIENT
        buffer[3] = engineAmbient;

        // Byte 4: OIL_PRESSURE
        buffer[4] = oilPressure;

        // Byte 5: RPM
        buffer[5] = rpm;

        // Byte 6: warnings
        buffer[6] = warnings;

        // Byte 7: ALERTS bits (8 bits)
        buffer[7] = data.alerts;
    }

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
     * Pack a value with warning bit into 8 bits (with alert tracking)
     */
    static uint8_t packValue(int16_t value, bool warning, int16_t offset, uint8_t step, uint8_t& alert, uint8_t alertBit)
    {
        value = (value - offset) / step;
        uint8_t packedValue = constrain(value, 0, 255);
        if (warning) {
            alert |= (alertBit);  // Set bit alertBit
        }
        return packedValue;
    }

    /**
     * Pack a value into 8 bits (without alert tracking)
     */
    static uint8_t packValue(int16_t value, bool warning, int16_t offset, uint8_t step)
    {
        value = (value - offset) / step;
        uint8_t packedValue = constrain(value, 0, 255);
        return packedValue;
    }

    /**
     * Unpack a value (with alert checking)
     */
    static void unpackValue(uint16_t packed, int16_t& value, bool& warning, int16_t offset, uint8_t step, uint8_t alert, uint8_t alertBit) {
        value = packed & 0x03FF;  // Lower 10 bits
        warning = (alert & alertBit) != 0;  // Bit alertBit
        value = (value * step) + offset;
    }   
};

} // namespace CanbusLogging

#endif // ENGINE_DATA_1_PACKER_H
