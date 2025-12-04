#include <unity.h>
#include <EngineData1Packer.h>

using namespace CanbusLogging;

// Helper function to compare two EngineData1 structures
bool compareEngineData(const EngineData1& a, const EngineData1& b, int16_t tolerance = 0) {
    // Due to quantization, we allow some tolerance in temperature comparisons
    bool result = true;
    result &= (abs(a.egt1Celsius - b.egt1Celsius) <= tolerance);
    result &= (a.egt1OverTemp == b.egt1OverTemp);
    result &= (abs(a.cht1Celsius - b.cht1Celsius) <= tolerance);
    result &= (a.cht1OverTemp == b.cht1OverTemp);
    result &= (abs(a.oilTempCelsius - b.oilTempCelsius) <= tolerance);
    result &= (a.oilTempOverTemp == b.oilTempOverTemp);
    result &= (abs(a.engineAmbientCelsius - b.engineAmbientCelsius) <= tolerance);
    result &= (a.engineAmbientOverTemp == b.engineAmbientOverTemp);
    result &= (abs(a.oilPressureX10 - b.oilPressureX10) <= tolerance);
    result &= (a.oilPressureUnder == b.oilPressureUnder);
    // RPM is uint16_t, so use absolute difference for unsigned types
    uint16_t rpmDiff = (a.rpm > b.rpm) ? (a.rpm - b.rpm) : (b.rpm - a.rpm);
    result &= (rpmDiff <= (uint16_t)tolerance);
    result &= (a.alerts == b.alerts);
    return result;
}

// Test: Pack and unpack with normal values
void test_pack_unpack_normal_values() {
    EngineData1 original = {
        .egt1Celsius = 500,
        .egt1OverTemp = false,
        .cht1Celsius = 150,
        .cht1OverTemp = false,
        .oilTempCelsius = 90,
        .oilTempOverTemp = false,
        .engineAmbientCelsius = 25,
        .engineAmbientOverTemp = false,
        .oilPressureX10 = 45,  // 4.5 bar
        .oilPressureUnder = false,
        .rpm = 2400,
        .alerts = ALERT_NONE
    };

    uint8_t buffer[8];
    EngineData1Packer::pack(original, buffer);

    EngineData1 unpacked;
    EngineData1Packer::unpack(buffer, unpacked);

    // Due to quantization, allow tolerance:
    // EGT: step 2°C, CHT/OIL: step 1°C, ENGINE_AMBIENT: step 3°C, RPM: step 20
    TEST_ASSERT_TRUE(abs(unpacked.egt1Celsius - original.egt1Celsius) <= 2);
    TEST_ASSERT_EQUAL(original.egt1OverTemp, unpacked.egt1OverTemp);

    TEST_ASSERT_TRUE(abs(unpacked.cht1Celsius - original.cht1Celsius) <= 1);
    TEST_ASSERT_EQUAL(original.cht1OverTemp, unpacked.cht1OverTemp);

    TEST_ASSERT_TRUE(abs(unpacked.oilTempCelsius - original.oilTempCelsius) <= 1);
    TEST_ASSERT_EQUAL(original.oilTempOverTemp, unpacked.oilTempOverTemp);

    TEST_ASSERT_TRUE(abs(unpacked.engineAmbientCelsius - original.engineAmbientCelsius) <= 3);
    TEST_ASSERT_EQUAL(original.engineAmbientOverTemp, unpacked.engineAmbientOverTemp);

    TEST_ASSERT_TRUE(abs(unpacked.oilPressureX10 - original.oilPressureX10) <= 1);
    TEST_ASSERT_EQUAL(original.oilPressureUnder, unpacked.oilPressureUnder);

    TEST_ASSERT_TRUE(abs(unpacked.rpm - original.rpm) <= 20);
    TEST_ASSERT_EQUAL(original.alerts, unpacked.alerts);
}

// Test: Pack with warning flags set
void test_pack_with_warnings() {
    EngineData1 data = {
        .egt1Celsius = 700,
        .egt1OverTemp = true,  // Warning flag set
        .cht1Celsius = 250,
        .cht1OverTemp = true,  // Warning flag set
        .oilTempCelsius = 120,
        .oilTempOverTemp = false,
        .engineAmbientCelsius = 50,
        .engineAmbientOverTemp = false,
        .oilPressureX10 = 15,
        .oilPressureUnder = true,  // Warning flag set
        .rpm = 3000,
        .alerts = ALERT_EGT_HIGH | ALERT_CHT_HIGH | ALERT_OIL_PRESSURE_LOW
    };

    uint8_t buffer[8];
    EngineData1Packer::pack(data, buffer);

    // Check that warning bits are set in byte 6
    TEST_ASSERT_TRUE((buffer[6] & ALERT_EGT_HIGH) != 0);
    TEST_ASSERT_TRUE((buffer[6] & ALERT_CHT_HIGH) != 0);
    TEST_ASSERT_TRUE((buffer[6] & ALERT_OIL_PRESSURE_LOW) != 0);

    // Check alerts in byte 7
    TEST_ASSERT_EQUAL(data.alerts, buffer[7]);

    EngineData1 unpacked;
    EngineData1Packer::unpack(buffer, unpacked);

    TEST_ASSERT_TRUE(unpacked.egt1OverTemp);
    TEST_ASSERT_TRUE(unpacked.cht1OverTemp);
    TEST_ASSERT_TRUE(unpacked.oilPressureUnder);
    TEST_ASSERT_EQUAL(data.alerts, unpacked.alerts);
}

// Test: EGT clamping at minimum (250°C)
void test_egt_clamping_minimum() {
    EngineData1 data = {
        .egt1Celsius = 200,  // Below minimum
        .egt1OverTemp = false,
        .cht1Celsius = 100,
        .cht1OverTemp = false,
        .oilTempCelsius = 80,
        .oilTempOverTemp = false,
        .engineAmbientCelsius = 20,
        .engineAmbientOverTemp = false,
        .oilPressureX10 = 40,
        .oilPressureUnder = false,
        .rpm = 2000,
        .alerts = ALERT_NONE
    };

    uint8_t buffer[8];
    EngineData1Packer::pack(data, buffer);

    // Byte 0 should be 0 (clamped to minimum)
    TEST_ASSERT_EQUAL_UINT8(0, buffer[0]);

    EngineData1 unpacked;
    EngineData1Packer::unpack(buffer, unpacked);

    // Unpacked value should be at the minimum (250°C)
    TEST_ASSERT_EQUAL_INT16(250, unpacked.egt1Celsius);
}

// Test: EGT clamping at maximum (762°C)
void test_egt_clamping_maximum() {
    EngineData1 data = {
        .egt1Celsius = 800,  // Above maximum
        .egt1OverTemp = false,
        .cht1Celsius = 100,
        .cht1OverTemp = false,
        .oilTempCelsius = 80,
        .oilTempOverTemp = false,
        .engineAmbientCelsius = 20,
        .engineAmbientOverTemp = false,
        .oilPressureX10 = 40,
        .oilPressureUnder = false,
        .rpm = 2000,
        .alerts = ALERT_NONE
    };

    uint8_t buffer[8];
    EngineData1Packer::pack(data, buffer);

    // Byte 0 should be 255 (clamped to maximum)
    TEST_ASSERT_EQUAL_UINT8(255, buffer[0]);

    EngineData1 unpacked;
    EngineData1Packer::unpack(buffer, unpacked);

    // Unpacked value should be at the maximum (762°C = 250 + 255*2)
    TEST_ASSERT_EQUAL_INT16(760, unpacked.egt1Celsius);
}

// Test: CHT clamping at minimum (50°C)
void test_cht_clamping_minimum() {
    EngineData1 data = {
        .egt1Celsius = 400,
        .egt1OverTemp = false,
        .cht1Celsius = 20,  // Below minimum
        .cht1OverTemp = false,
        .oilTempCelsius = 80,
        .oilTempOverTemp = false,
        .engineAmbientCelsius = 20,
        .engineAmbientOverTemp = false,
        .oilPressureX10 = 40,
        .oilPressureUnder = false,
        .rpm = 2000,
        .alerts = ALERT_NONE
    };

    uint8_t buffer[8];
    EngineData1Packer::pack(data, buffer);

    // Byte 1 should be 0 (clamped to minimum)
    TEST_ASSERT_EQUAL_UINT8(0, buffer[1]);

    EngineData1 unpacked;
    EngineData1Packer::unpack(buffer, unpacked);

    // Unpacked value should be at the minimum (50°C)
    TEST_ASSERT_EQUAL_INT16(50, unpacked.cht1Celsius);
}

// Test: CHT clamping at maximum (306°C)
void test_cht_clamping_maximum() {
    EngineData1 data = {
        .egt1Celsius = 400,
        .egt1OverTemp = false,
        .cht1Celsius = 350,  // Above maximum
        .cht1OverTemp = false,
        .oilTempCelsius = 80,
        .oilTempOverTemp = false,
        .engineAmbientCelsius = 20,
        .engineAmbientOverTemp = false,
        .oilPressureX10 = 40,
        .oilPressureUnder = false,
        .rpm = 2000,
        .alerts = ALERT_NONE
    };

    uint8_t buffer[8];
    EngineData1Packer::pack(data, buffer);

    // Byte 1 should be 255 (clamped to maximum)
    TEST_ASSERT_EQUAL_UINT8(255, buffer[1]);

    EngineData1 unpacked;
    EngineData1Packer::unpack(buffer, unpacked);

    // Unpacked value should be at the maximum (305°C = 50 + 255*1)
    TEST_ASSERT_EQUAL_INT16(305, unpacked.cht1Celsius);
}

// Test: Engine ambient temperature with negative values
void test_engine_ambient_negative() {
    EngineData1 data = {
        .egt1Celsius = 400,
        .egt1OverTemp = false,
        .cht1Celsius = 100,
        .cht1OverTemp = false,
        .oilTempCelsius = 80,
        .oilTempOverTemp = false,
        .engineAmbientCelsius = -20,
        .engineAmbientOverTemp = false,
        .oilPressureX10 = 40,
        .oilPressureUnder = false,
        .rpm = 2000,
        .alerts = ALERT_NONE
    };

    uint8_t buffer[8];
    EngineData1Packer::pack(data, buffer);

    EngineData1 unpacked;
    EngineData1Packer::unpack(buffer, unpacked);

    // Should be within step tolerance (3°C)
    TEST_ASSERT_TRUE(abs(unpacked.engineAmbientCelsius - data.engineAmbientCelsius) <= 3);
}

// Test: Engine ambient clamping at minimum (-50°C)
void test_engine_ambient_clamping_minimum() {
    EngineData1 data = {
        .egt1Celsius = 400,
        .egt1OverTemp = false,
        .cht1Celsius = 100,
        .cht1OverTemp = false,
        .oilTempCelsius = 80,
        .oilTempOverTemp = false,
        .engineAmbientCelsius = -100,  // Below minimum
        .engineAmbientOverTemp = false,
        .oilPressureX10 = 40,
        .oilPressureUnder = false,
        .rpm = 2000,
        .alerts = ALERT_NONE
    };

    uint8_t buffer[8];
    EngineData1Packer::pack(data, buffer);

    // Byte 3 should be 0 (clamped to minimum)
    TEST_ASSERT_EQUAL_UINT8(0, buffer[3]);

    EngineData1 unpacked;
    EngineData1Packer::unpack(buffer, unpacked);

    // Unpacked value should be at the minimum (-50°C)
    TEST_ASSERT_EQUAL_INT16(-50, unpacked.engineAmbientCelsius);
}

// Test: RPM packing with step of 20
void test_rpm_packing() {
    EngineData1 data = {
        .egt1Celsius = 400,
        .egt1OverTemp = false,
        .cht1Celsius = 100,
        .cht1OverTemp = false,
        .oilTempCelsius = 80,
        .oilTempOverTemp = false,
        .engineAmbientCelsius = 20,
        .engineAmbientOverTemp = false,
        .oilPressureX10 = 40,
        .oilPressureUnder = false,
        .rpm = 2850,  // Should pack to 142 (2850/20 = 142.5)
        .alerts = ALERT_NONE
    };

    uint8_t buffer[8];
    EngineData1Packer::pack(data, buffer);

    // Byte 5 should be 142
    TEST_ASSERT_EQUAL_UINT8(142, buffer[5]);

    EngineData1 unpacked;
    EngineData1Packer::unpack(buffer, unpacked);

    // Unpacked value should be 2840 (142 * 20)
    TEST_ASSERT_EQUAL_UINT16(2840, unpacked.rpm);
}

// Test: RPM clamping at maximum (5100 RPM)
void test_rpm_clamping_maximum() {
    EngineData1 data = {
        .egt1Celsius = 400,
        .egt1OverTemp = false,
        .cht1Celsius = 100,
        .cht1OverTemp = false,
        .oilTempCelsius = 80,
        .oilTempOverTemp = false,
        .engineAmbientCelsius = 20,
        .engineAmbientOverTemp = false,
        .oilPressureX10 = 40,
        .oilPressureUnder = false,
        .rpm = 6000,  // Above maximum
        .alerts = ALERT_NONE
    };

    uint8_t buffer[8];
    EngineData1Packer::pack(data, buffer);

    // Byte 5 should be 255 (clamped)
    TEST_ASSERT_EQUAL_UINT8(255, buffer[5]);

    EngineData1 unpacked;
    EngineData1Packer::unpack(buffer, unpacked);

    // Unpacked value should be 5100 (255 * 20)
    TEST_ASSERT_EQUAL_UINT16(5100, unpacked.rpm);
}

// Test: Oil pressure packing
void test_oil_pressure_packing() {
    EngineData1 data = {
        .egt1Celsius = 400,
        .egt1OverTemp = false,
        .cht1Celsius = 100,
        .cht1OverTemp = false,
        .oilTempCelsius = 80,
        .oilTempOverTemp = false,
        .engineAmbientCelsius = 20,
        .engineAmbientOverTemp = false,
        .oilPressureX10 = 47,  // 4.7 bar
        .oilPressureUnder = false,
        .rpm = 2000,
        .alerts = ALERT_NONE
    };

    uint8_t buffer[8];
    EngineData1Packer::pack(data, buffer);

    EngineData1 unpacked;
    EngineData1Packer::unpack(buffer, unpacked);

    // Should match exactly (step 1)
    TEST_ASSERT_EQUAL_INT16(47, unpacked.oilPressureX10);
    TEST_ASSERT_FALSE(unpacked.oilPressureUnder);
}

// Test: All alerts combined
void test_multiple_alerts() {
    EngineData1 data = {
        .egt1Celsius = 400,
        .egt1OverTemp = false,
        .cht1Celsius = 100,
        .cht1OverTemp = false,
        .oilTempCelsius = 80,
        .oilTempOverTemp = false,
        .engineAmbientCelsius = 20,
        .engineAmbientOverTemp = false,
        .oilPressureX10 = 40,
        .oilPressureUnder = false,
        .rpm = 2000,
        .alerts = ALERT_EGT_HIGH | ALERT_CHT_HIGH | ALERT_OIL_TEMP_HIGH |
                  ALERT_OIL_PRESSURE_LOW | ALERT_ENGINE_OVERHEAT | ALERT_SENSOR_FAILURE
    };

    uint8_t buffer[8];
    EngineData1Packer::pack(data, buffer);

    EngineData1 unpacked;
    EngineData1Packer::unpack(buffer, unpacked);

    TEST_ASSERT_EQUAL_UINT8(data.alerts, unpacked.alerts);
}

// Test: Zero values
void test_zero_values() {
    EngineData1 data = {
        .egt1Celsius = 250,  // Minimum for EGT
        .egt1OverTemp = false,
        .cht1Celsius = 50,   // Minimum for CHT
        .cht1OverTemp = false,
        .oilTempCelsius = 50,  // Minimum for oil temp
        .oilTempOverTemp = false,
        .engineAmbientCelsius = -50,  // Minimum for ambient
        .engineAmbientOverTemp = false,
        .oilPressureX10 = 0,
        .oilPressureUnder = false,
        .rpm = 0,
        .alerts = ALERT_NONE
    };

    uint8_t buffer[8];
    EngineData1Packer::pack(data, buffer);

    // All temperature bytes should be 0
    TEST_ASSERT_EQUAL_UINT8(0, buffer[0]);  // EGT
    TEST_ASSERT_EQUAL_UINT8(0, buffer[1]);  // CHT
    TEST_ASSERT_EQUAL_UINT8(0, buffer[2]);  // OIL_TEMP
    TEST_ASSERT_EQUAL_UINT8(0, buffer[3]);  // ENGINE_AMBIENT
    TEST_ASSERT_EQUAL_UINT8(0, buffer[4]);  // OIL_PRESSURE
    TEST_ASSERT_EQUAL_UINT8(0, buffer[5]);  // RPM

    EngineData1 unpacked;
    EngineData1Packer::unpack(buffer, unpacked);

    TEST_ASSERT_EQUAL_INT16(250, unpacked.egt1Celsius);
    TEST_ASSERT_EQUAL_INT16(50, unpacked.cht1Celsius);
    TEST_ASSERT_EQUAL_INT16(50, unpacked.oilTempCelsius);
    TEST_ASSERT_EQUAL_INT16(-50, unpacked.engineAmbientCelsius);
    TEST_ASSERT_EQUAL_INT16(0, unpacked.oilPressureX10);
    TEST_ASSERT_EQUAL_UINT16(0, unpacked.rpm);
}

void setup() {
    delay(2000);  // Wait for serial monitor
    UNITY_BEGIN();

    RUN_TEST(test_pack_unpack_normal_values);
    RUN_TEST(test_pack_with_warnings);
    RUN_TEST(test_egt_clamping_minimum);
    RUN_TEST(test_egt_clamping_maximum);
    RUN_TEST(test_cht_clamping_minimum);
    RUN_TEST(test_cht_clamping_maximum);
    RUN_TEST(test_engine_ambient_negative);
    RUN_TEST(test_engine_ambient_clamping_minimum);
    RUN_TEST(test_rpm_packing);
    RUN_TEST(test_rpm_clamping_maximum);
    RUN_TEST(test_oil_pressure_packing);
    RUN_TEST(test_multiple_alerts);
    RUN_TEST(test_zero_values);

    UNITY_END();
}

void loop() {
    // Nothing here - tests run once in setup()
}
