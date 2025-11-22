#ifndef ENGINE_DATA_1_PACKER_H
#define ENGINE_DATA_1_PACKER_H

#include <Arduino.h>

namespace CanbusLogging {

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
    // Bits 6-11 reserved for future use
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

    uint8_t oilPressureX10;  // Pressure in bar * 10 (0-120)
    bool oilPressureUnder;

    uint16_t alerts;  // 12-bit alert flags
};

/**
 * Packer/unpacker for ENGINE_DATA_1 CAN message
 *
 * Packs engine sensor data into 8-byte CAN frame:
 * - EGT1: 11 bits (10 bits value 0-1023 + 1 bit over-temp)
 * - CHT1: 11 bits (10 bits value 0-1023 + 1 bit over-temp)
 * - OIL_TEMP: 11 bits (10 bits value 0-1023 + 1 bit over-temp)
 * - ENGINE_AMBIENT: 11 bits (10 bits value 0-1023 + 1 bit over-temp)
 * - OIL_PRESSURE: 8 bits (7 bits value 0-127 + 1 bit under-pressure)
 * - ALERTS: 12 bits
 *
 * Bit layout in 8 bytes (64 bits):
 * Byte 0: EGT1[0-7]
 * Byte 1: EGT1[8-10] | CHT1[0-4]
 * Byte 2: CHT1[5-10] | OIL_TEMP[0-1]
 * Byte 3: OIL_TEMP[2-9]
 * Byte 4: OIL_TEMP[10] | ENGINE_AMBIENT[0-6]
 * Byte 5: ENGINE_AMBIENT[7-10] | OIL_PRESSURE[0-3]
 * Byte 6: OIL_PRESSURE[4-7] | ALERTS[0-3]
 * Byte 7: ALERTS[4-11]
 */
class EngineData1Packer {
public:
    /**
     * Pack engine data into 8-byte buffer
     * @param data Source data structure
     * @param buffer 8-byte output buffer
     */
    static void pack(const EngineData1& data, uint8_t* buffer) {
        uint16_t egt1 = packTemperature(data.egt1Celsius, data.egt1OverTemp);
        uint16_t cht1 = packTemperature(data.cht1Celsius, data.cht1OverTemp);
        uint16_t oilTemp = packTemperature(data.oilTempCelsius, data.oilTempOverTemp);
        uint16_t engineAmbient = packTemperature(data.engineAmbientCelsius, data.engineAmbientOverTemp);
        uint8_t oilPressure = packOilPressure(data.oilPressureX10, data.oilPressureUnder);
        uint16_t alerts = data.alerts & 0x0FFF;

        // Byte 0: EGT1 bits 0-7
        buffer[0] = egt1 & 0xFF;

        // Byte 1: EGT1 bits 8-10 (3 bits) | CHT1 bits 0-4 (5 bits)
        buffer[1] = ((egt1 >> 8) & 0x07) | ((cht1 & 0x1F) << 3);

        // Byte 2: CHT1 bits 5-10 (6 bits) | OIL_TEMP bits 0-1 (2 bits)
        buffer[2] = ((cht1 >> 5) & 0x3F) | ((oilTemp & 0x03) << 6);

        // Byte 3: OIL_TEMP bits 2-9 (8 bits)
        buffer[3] = (oilTemp >> 2) & 0xFF;

        // Byte 4: OIL_TEMP bit 10 (1 bit) | ENGINE_AMBIENT bits 0-6 (7 bits)
        buffer[4] = ((oilTemp >> 10) & 0x01) | ((engineAmbient & 0x7F) << 1);

        // Byte 5: ENGINE_AMBIENT bits 7-10 (4 bits) | OIL_PRESSURE bits 0-3 (4 bits)
        buffer[5] = ((engineAmbient >> 7) & 0x0F) | ((oilPressure & 0x0F) << 4);

        // Byte 6: OIL_PRESSURE bits 4-7 (4 bits) | ALERTS bits 0-3 (4 bits)
        buffer[6] = ((oilPressure >> 4) & 0x0F) | ((alerts & 0x0F) << 4);

        // Byte 7: ALERTS bits 4-11 (8 bits)
        buffer[7] = (alerts >> 4) & 0xFF;
    }

    /**
     * Unpack 8-byte buffer into engine data structure
     * @param buffer 8-byte input buffer
     * @param data Output data structure
     */
    static void unpack(const uint8_t* buffer, EngineData1& data) {
        // Reconstruct packed values
        uint16_t egt1 = buffer[0] | ((buffer[1] & 0x07) << 8);
        uint16_t cht1 = ((buffer[1] >> 3) & 0x1F) | ((buffer[2] & 0x3F) << 5);
        uint16_t oilTemp = ((buffer[2] >> 6) & 0x03) | (buffer[3] << 2) | ((buffer[4] & 0x01) << 10);
        uint16_t engineAmbient = ((buffer[4] >> 1) & 0x7F) | ((buffer[5] & 0x0F) << 7);
        uint8_t oilPressure = ((buffer[5] >> 4) & 0x0F) | ((buffer[6] & 0x0F) << 4);
        uint16_t alerts = ((buffer[6] >> 4) & 0x0F) | (buffer[7] << 4);

        // Unpack temperatures
        unpackTemperature(egt1, data.egt1Celsius, data.egt1OverTemp);
        unpackTemperature(cht1, data.cht1Celsius, data.cht1OverTemp);
        unpackTemperature(oilTemp, data.oilTempCelsius, data.oilTempOverTemp);
        unpackTemperature(engineAmbient, data.engineAmbientCelsius, data.engineAmbientOverTemp);

        // Unpack oil pressure
        unpackOilPressure(oilPressure, data.oilPressureX10, data.oilPressureUnder);

        // Alerts
        data.alerts = alerts;
    }

private:
    /**
     * Pack a temperature value with warning bit into 11 bits
     */
    static uint16_t packTemperature(int16_t celsius, bool overTemp) {
        uint16_t value = constrain(celsius, 0, 1023);
        if (overTemp) {
            value |= 0x0400;  // Set bit 10
        }
        return value;
    }

    /**
     * Unpack an 11-bit temperature value
     */
    static void unpackTemperature(uint16_t packed, int16_t& celsius, bool& overTemp) {
        celsius = packed & 0x03FF;  // Lower 10 bits
        overTemp = (packed & 0x0400) != 0;  // Bit 10
    }

    /**
     * Pack oil pressure value with warning bit into 8 bits
     */
    static uint8_t packOilPressure(uint8_t pressureX10, bool underPressure) {
        uint8_t value = constrain(pressureX10, 0, 127);
        if (underPressure) {
            value |= 0x80;  // Set bit 7
        }
        return value;
    }

    /**
     * Unpack an 8-bit oil pressure value
     */
    static void unpackOilPressure(uint8_t packed, uint8_t& pressureX10, bool& underPressure) {
        pressureX10 = packed & 0x7F;  // Lower 7 bits
        underPressure = (packed & 0x80) != 0;  // Bit 7
    }
};

} // namespace CanbusLogging

#endif // ENGINE_DATA_1_PACKER_H
