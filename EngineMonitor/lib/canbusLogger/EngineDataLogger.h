#ifndef ENGINE_DATA_LOGGER_H
#define ENGINE_DATA_LOGGER_H

#include "CanbusLogger.h"
#include "EngineData1Packer.h"

namespace CanbusLogging {

/**
 * Engine data logger for ENGINE_DATA_1 CAN message
 *
 * Uses EngineData1Packer for data serialization.
 */
class EngineDataLogger : public CanbusLogger {
public:
    /**
     * Constructor
     * @param csPin Chip select pin for MCP2515
     */
    EngineDataLogger(uint8_t csPin) : CanbusLogger(csPin) {
        clearData();
    }

    /**
     * Get message type
     * @return ENGINE_DATA_1 message ID
     */
    uint16_t getMessageType() const override {
        return ENGINE_DATA_1;
    }

    /**
     * Set EGT1 temperature
     * @param celsius Temperature in degrees Celsius
     * @param overTemp Over-temperature warning flag
     */
    void setEGT1(int16_t celsius, bool overTemp = false) {
        _data.egt1Celsius = celsius;
        _data.egt1OverTemp = overTemp;
    }

    /**
     * Set CHT1 temperature
     * @param celsius Temperature in degrees Celsius
     * @param overTemp Over-temperature warning flag
     */
    void setCHT1(int16_t celsius, bool overTemp = false) {
        _data.cht1Celsius = celsius;
        _data.cht1OverTemp = overTemp;
    }

    /**
     * Set oil temperature
     * @param celsius Temperature in degrees Celsius
     * @param overTemp Over-temperature warning flag
     */
    void setOilTemp(int16_t celsius, bool overTemp = false) {
        _data.oilTempCelsius = celsius;
        _data.oilTempOverTemp = overTemp;
    }

    /**
     * Set engine ambient temperature
     * @param celsius Temperature in degrees Celsius
     * @param overTemp Over-temperature warning flag
     */
    void setEngineAmbient(int16_t celsius, bool overTemp = false) {
        _data.engineAmbientCelsius = celsius;
        _data.engineAmbientOverTemp = overTemp;
    }

    /**
     * Set oil pressure
     * @param pressureX10 Pressure in bar * 10 (e.g., 45 = 4.5 bar)
     * @param underPressure Under-pressure warning flag
     */
    void setOilPressure(uint8_t pressureX10, bool underPressure = false) {
        _data.oilPressureX10 = pressureX10;
        _data.oilPressureUnder = underPressure;
    }

    /**
     * Set engine RPM
     * @param rpm Engine RPM (0-5000)
     */
    void setRPM(int16_t rpm) {
        _data.rpm = rpm;
    }

    /**
     * Set alert flags
     * @param alerts Combination of EngineAlerts flags
     */
    void setAlertFlag(uint8_t alertsFlag, bool enable) {
        if (enable) {
            _data.alerts |= alertsFlag;
        } else {
            _data.alerts &= ~alertsFlag;
        }
    }

    /**
     * Clear all data to defaults
     */
    void clearData() {
        _data = EngineData1();
    }

    /**
     * Get current data structure
     * @return Reference to internal data
     */
    const EngineData1& getData() const {
        return _data;
    }

    /**
     * Set data from structure
     * @param data Data to copy
     */
    void setData(const EngineData1& data) {
        _data = data;
    }

    /**
     * Pack and send the ENGINE_DATA_1 frame
     * @return true if sent successfully
     */
    bool sendFrame() override {
        uint8_t buffer[8];
        EngineData1Packer::pack(_data, buffer);
        return sendMessage(ENGINE_DATA_1, buffer);
    }

private:
    EngineData1 _data;
};

} // namespace CanbusLogging

#endif // ENGINE_DATA_LOGGER_H
