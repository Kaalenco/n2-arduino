#include <unity.h>
#include <EngineDataLogger.h>

using namespace CanbusLogging;

// Test fixture
EngineDataLogger* logger = nullptr;

void setUp() {
    // Create a new logger for each test
    logger = new EngineDataLogger();
}

void tearDown() {
    // Clean up after each test
    delete logger;
    logger = nullptr;
}

// Test: Constructor initializes with cleared data
void test_constructor_clears_data() {
    const EngineData1& data = logger->getData();

    // All values should be zero/false after construction
    TEST_ASSERT_EQUAL_INT16(0, data.egt1Celsius);
    TEST_ASSERT_FALSE(data.egt1OverTemp);
    TEST_ASSERT_EQUAL_INT16(0, data.cht1Celsius);
    TEST_ASSERT_FALSE(data.cht1OverTemp);
    TEST_ASSERT_EQUAL_INT16(0, data.oilTempCelsius);
    TEST_ASSERT_FALSE(data.oilTempOverTemp);
    TEST_ASSERT_EQUAL_INT16(0, data.engineAmbientCelsius);
    TEST_ASSERT_FALSE(data.engineAmbientOverTemp);
    TEST_ASSERT_EQUAL_INT16(0, data.oilPressureX10);
    TEST_ASSERT_FALSE(data.oilPressureUnder);
    TEST_ASSERT_EQUAL_UINT16(0, data.rpm);
    TEST_ASSERT_EQUAL_UINT8(0, data.alerts);
}

// Test: getMessageType returns ENGINE_DATA_1
void test_get_message_type() {
    TEST_ASSERT_EQUAL_UINT16(ENGINE_DATA_1, logger->getMessageType());
    TEST_ASSERT_EQUAL_UINT16(0x100, logger->getMessageType());
}

// Test: setEGT1 with normal values
void test_set_egt1_normal() {
    logger->setEGT1(500, false);

    const EngineData1& data = logger->getData();
    TEST_ASSERT_EQUAL_INT16(500, data.egt1Celsius);
    TEST_ASSERT_FALSE(data.egt1OverTemp);
}

// Test: setEGT1 with overtemp warning
void test_set_egt1_with_warning() {
    logger->setEGT1(750, true);

    const EngineData1& data = logger->getData();
    TEST_ASSERT_EQUAL_INT16(750, data.egt1Celsius);
    TEST_ASSERT_TRUE(data.egt1OverTemp);
}

// Test: setEGT1 boundary values
void test_set_egt1_boundary_values() {
    // Minimum expected value
    logger->setEGT1(250);
    TEST_ASSERT_EQUAL_INT16(250, logger->getData().egt1Celsius);

    // Maximum expected value
    logger->setEGT1(762);
    TEST_ASSERT_EQUAL_INT16(762, logger->getData().egt1Celsius);

    // Beyond maximum (will be clamped by packer)
    logger->setEGT1(1000);
    TEST_ASSERT_EQUAL_INT16(1000, logger->getData().egt1Celsius);  // Stored as-is, clamped during pack
}

// Test: setCHT1 with normal values
void test_set_cht1_normal() {
    logger->setCHT1(150, false);

    const EngineData1& data = logger->getData();
    TEST_ASSERT_EQUAL_INT16(150, data.cht1Celsius);
    TEST_ASSERT_FALSE(data.cht1OverTemp);
}

// Test: setCHT1 with overtemp warning
void test_set_cht1_with_warning() {
    logger->setCHT1(280, true);

    const EngineData1& data = logger->getData();
    TEST_ASSERT_EQUAL_INT16(280, data.cht1Celsius);
    TEST_ASSERT_TRUE(data.cht1OverTemp);
}

// Test: setOilTemp with normal values
void test_set_oil_temp_normal() {
    logger->setOilTemp(95, false);

    const EngineData1& data = logger->getData();
    TEST_ASSERT_EQUAL_INT16(95, data.oilTempCelsius);
    TEST_ASSERT_FALSE(data.oilTempOverTemp);
}

// Test: setOilTemp with overtemp warning
void test_set_oil_temp_with_warning() {
    logger->setOilTemp(120, true);

    const EngineData1& data = logger->getData();
    TEST_ASSERT_EQUAL_INT16(120, data.oilTempCelsius);
    TEST_ASSERT_TRUE(data.oilTempOverTemp);
}

// Test: setEngineAmbient with normal values
void test_set_engine_ambient_normal() {
    logger->setEngineAmbient(25, false);

    const EngineData1& data = logger->getData();
    TEST_ASSERT_EQUAL_INT16(25, data.engineAmbientCelsius);
    TEST_ASSERT_FALSE(data.engineAmbientOverTemp);
}

// Test: setEngineAmbient with negative temperature
void test_set_engine_ambient_negative() {
    logger->setEngineAmbient(-20, false);

    const EngineData1& data = logger->getData();
    TEST_ASSERT_EQUAL_INT16(-20, data.engineAmbientCelsius);
    TEST_ASSERT_FALSE(data.engineAmbientOverTemp);
}

// Test: setEngineAmbient with overtemp warning
void test_set_engine_ambient_with_warning() {
    logger->setEngineAmbient(50, true);

    const EngineData1& data = logger->getData();
    TEST_ASSERT_EQUAL_INT16(50, data.engineAmbientCelsius);
    TEST_ASSERT_TRUE(data.engineAmbientOverTemp);
}

// Test: setOilPressure with normal values
void test_set_oil_pressure_normal() {
    logger->setOilPressure(45, false);  // 4.5 bar

    const EngineData1& data = logger->getData();
    TEST_ASSERT_EQUAL_UINT8(45, data.oilPressureX10);
    TEST_ASSERT_FALSE(data.oilPressureUnder);
}

// Test: setOilPressure with under-pressure warning
void test_set_oil_pressure_with_warning() {
    logger->setOilPressure(15, true);  // 1.5 bar - low pressure

    const EngineData1& data = logger->getData();
    TEST_ASSERT_EQUAL_UINT8(15, data.oilPressureX10);
    TEST_ASSERT_TRUE(data.oilPressureUnder);
}

// Test: setOilPressure boundary values
void test_set_oil_pressure_boundary() {
    // Minimum
    logger->setOilPressure(0);
    TEST_ASSERT_EQUAL_UINT8(0, logger->getData().oilPressureX10);

    // Maximum expected (12.0 bar)
    logger->setOilPressure(120);
    TEST_ASSERT_EQUAL_UINT8(120, logger->getData().oilPressureX10);
}

// Test: setRPM with normal values
void test_set_rpm_normal() {
    logger->setRPM(2400);

    const EngineData1& data = logger->getData();
    TEST_ASSERT_EQUAL_INT16(2400, data.rpm);
}

// Test: setRPM boundary values
void test_set_rpm_boundary() {
    // Zero RPM
    logger->setRPM(0);
    TEST_ASSERT_EQUAL_INT16(0, logger->getData().rpm);

    // Typical maximum
    logger->setRPM(5000);
    TEST_ASSERT_EQUAL_INT16(5000, logger->getData().rpm);

    // Beyond typical (will be clamped by packer)
    logger->setRPM(6000);
    TEST_ASSERT_EQUAL_INT16(6000, logger->getData().rpm);  // Stored as-is, clamped during pack
}

// Test: setAlertFlag enables single flag
void test_set_alert_flag_enable_single() {
    logger->setAlertFlag(ALERT_EGT_HIGH, true);

    const EngineData1& data = logger->getData();
    TEST_ASSERT_EQUAL_UINT8(ALERT_EGT_HIGH, data.alerts);
    TEST_ASSERT_TRUE((data.alerts & ALERT_EGT_HIGH) != 0);
}

// Test: setAlertFlag enables multiple flags
void test_set_alert_flag_enable_multiple() {
    logger->setAlertFlag(ALERT_EGT_HIGH, true);
    logger->setAlertFlag(ALERT_CHT_HIGH, true);
    logger->setAlertFlag(ALERT_OIL_PRESSURE_LOW, true);

    const EngineData1& data = logger->getData();
    TEST_ASSERT_EQUAL_UINT8(ALERT_EGT_HIGH | ALERT_CHT_HIGH | ALERT_OIL_PRESSURE_LOW, data.alerts);
    TEST_ASSERT_TRUE((data.alerts & ALERT_EGT_HIGH) != 0);
    TEST_ASSERT_TRUE((data.alerts & ALERT_CHT_HIGH) != 0);
    TEST_ASSERT_TRUE((data.alerts & ALERT_OIL_PRESSURE_LOW) != 0);
}

// Test: setAlertFlag disables single flag
void test_set_alert_flag_disable_single() {
    // First enable multiple flags
    logger->setAlertFlag(ALERT_EGT_HIGH, true);
    logger->setAlertFlag(ALERT_CHT_HIGH, true);
    logger->setAlertFlag(ALERT_OIL_PRESSURE_LOW, true);

    // Then disable one
    logger->setAlertFlag(ALERT_CHT_HIGH, false);

    const EngineData1& data = logger->getData();
    TEST_ASSERT_EQUAL_UINT8(ALERT_EGT_HIGH | ALERT_OIL_PRESSURE_LOW, data.alerts);
    TEST_ASSERT_TRUE((data.alerts & ALERT_EGT_HIGH) != 0);
    TEST_ASSERT_FALSE((data.alerts & ALERT_CHT_HIGH) != 0);
    TEST_ASSERT_TRUE((data.alerts & ALERT_OIL_PRESSURE_LOW) != 0);
}

// Test: setAlertFlag toggle flag on/off
void test_set_alert_flag_toggle() {
    // Enable
    logger->setAlertFlag(ALERT_SENSOR_FAILURE, true);
    TEST_ASSERT_TRUE((logger->getData().alerts & ALERT_SENSOR_FAILURE) != 0);

    // Disable
    logger->setAlertFlag(ALERT_SENSOR_FAILURE, false);
    TEST_ASSERT_FALSE((logger->getData().alerts & ALERT_SENSOR_FAILURE) != 0);

    // Enable again
    logger->setAlertFlag(ALERT_SENSOR_FAILURE, true);
    TEST_ASSERT_TRUE((logger->getData().alerts & ALERT_SENSOR_FAILURE) != 0);
}

// Test: setAlertFlag with all flags
void test_set_alert_flag_all_flags() {
    logger->setAlertFlag(ALERT_EGT_HIGH, true);
    logger->setAlertFlag(ALERT_CHT_HIGH, true);
    logger->setAlertFlag(ALERT_OIL_TEMP_HIGH, true);
    logger->setAlertFlag(ALERT_OIL_PRESSURE_LOW, true);
    logger->setAlertFlag(ALERT_ENGINE_OVERHEAT, true);
    logger->setAlertFlag(ALERT_SENSOR_FAILURE, true);

    const EngineData1& data = logger->getData();
    uint8_t expected = ALERT_EGT_HIGH | ALERT_CHT_HIGH | ALERT_OIL_TEMP_HIGH |
                       ALERT_OIL_PRESSURE_LOW | ALERT_ENGINE_OVERHEAT | ALERT_SENSOR_FAILURE;
    TEST_ASSERT_EQUAL_UINT8(expected, data.alerts);
}

// Test: clearData resets all values
void test_clear_data() {
    // Set various values
    logger->setEGT1(500, true);
    logger->setCHT1(150, true);
    logger->setOilTemp(95, true);
    logger->setEngineAmbient(25, true);
    logger->setOilPressure(45, true);
    logger->setRPM(2400);
    logger->setAlertFlag(ALERT_EGT_HIGH, true);
    logger->setAlertFlag(ALERT_CHT_HIGH, true);

    // Verify data was set
    const EngineData1& dataBefore = logger->getData();
    TEST_ASSERT_EQUAL_INT16(500, dataBefore.egt1Celsius);
    TEST_ASSERT_EQUAL_INT16(2400, dataBefore.rpm);

    // Clear data
    logger->clearData();

    // Verify all values are reset
    const EngineData1& dataAfter = logger->getData();
    TEST_ASSERT_EQUAL_INT16(0, dataAfter.egt1Celsius);
    TEST_ASSERT_FALSE(dataAfter.egt1OverTemp);
    TEST_ASSERT_EQUAL_INT16(0, dataAfter.cht1Celsius);
    TEST_ASSERT_FALSE(dataAfter.cht1OverTemp);
    TEST_ASSERT_EQUAL_INT16(0, dataAfter.oilTempCelsius);
    TEST_ASSERT_FALSE(dataAfter.oilTempOverTemp);
    TEST_ASSERT_EQUAL_INT16(0, dataAfter.engineAmbientCelsius);
    TEST_ASSERT_FALSE(dataAfter.engineAmbientOverTemp);
    TEST_ASSERT_EQUAL_INT16(0, dataAfter.oilPressureX10);
    TEST_ASSERT_FALSE(dataAfter.oilPressureUnder);
    TEST_ASSERT_EQUAL_UINT16(0, dataAfter.rpm);
    TEST_ASSERT_EQUAL_UINT8(0, dataAfter.alerts);
}

// Test: getData returns current data
void test_get_data() {
    // Set some values
    logger->setEGT1(500, true);
    logger->setCHT1(150, false);
    logger->setRPM(2400);

    // Get data
    const EngineData1& data = logger->getData();

    // Verify values match
    TEST_ASSERT_EQUAL_INT16(500, data.egt1Celsius);
    TEST_ASSERT_TRUE(data.egt1OverTemp);
    TEST_ASSERT_EQUAL_INT16(150, data.cht1Celsius);
    TEST_ASSERT_FALSE(data.cht1OverTemp);
    TEST_ASSERT_EQUAL_INT16(2400, data.rpm);
}

// Test: setData replaces entire data structure
void test_set_data() {
    // Create a data structure
    EngineData1 newData = {
        .egt1Celsius = 600,
        .egt1OverTemp = true,
        .cht1Celsius = 180,
        .cht1OverTemp = false,
        .oilTempCelsius = 100,
        .oilTempOverTemp = false,
        .engineAmbientCelsius = 30,
        .engineAmbientOverTemp = false,
        .oilPressureX10 = 50,
        .oilPressureUnder = false,
        .rpm = 3000,
        .alerts = ALERT_EGT_HIGH | ALERT_SENSOR_FAILURE
    };

    // Set the data
    logger->setData(newData);

    // Verify all fields match
    const EngineData1& data = logger->getData();
    TEST_ASSERT_EQUAL_INT16(600, data.egt1Celsius);
    TEST_ASSERT_TRUE(data.egt1OverTemp);
    TEST_ASSERT_EQUAL_INT16(180, data.cht1Celsius);
    TEST_ASSERT_FALSE(data.cht1OverTemp);
    TEST_ASSERT_EQUAL_INT16(100, data.oilTempCelsius);
    TEST_ASSERT_FALSE(data.oilTempOverTemp);
    TEST_ASSERT_EQUAL_INT16(30, data.engineAmbientCelsius);
    TEST_ASSERT_FALSE(data.engineAmbientOverTemp);
    TEST_ASSERT_EQUAL_UINT8(50, data.oilPressureX10);
    TEST_ASSERT_FALSE(data.oilPressureUnder);
    TEST_ASSERT_EQUAL_UINT16(3000, data.rpm);
    TEST_ASSERT_EQUAL_UINT8(ALERT_EGT_HIGH | ALERT_SENSOR_FAILURE, data.alerts);
}

// Test: setData then modify individual values
void test_set_data_then_modify() {
    // Create initial data
    EngineData1 initialData = {
        .egt1Celsius = 500,
        .egt1OverTemp = false,
        .cht1Celsius = 150,
        .cht1OverTemp = false,
        .oilTempCelsius = 90,
        .oilTempOverTemp = false,
        .engineAmbientCelsius = 25,
        .engineAmbientOverTemp = false,
        .oilPressureX10 = 45,
        .oilPressureUnder = false,
        .rpm = 2400,
        .alerts = ALERT_NONE
    };

    logger->setData(initialData);

    // Modify individual values
    logger->setEGT1(550, true);
    logger->setRPM(2600);
    logger->setAlertFlag(ALERT_EGT_HIGH, true);

    // Verify modifications
    const EngineData1& data = logger->getData();
    TEST_ASSERT_EQUAL_INT16(550, data.egt1Celsius);
    TEST_ASSERT_TRUE(data.egt1OverTemp);
    TEST_ASSERT_EQUAL_UINT16(2600, data.rpm);
    TEST_ASSERT_TRUE((data.alerts & ALERT_EGT_HIGH) != 0);

    // Verify other values unchanged
    TEST_ASSERT_EQUAL_INT16(150, data.cht1Celsius);
    TEST_ASSERT_EQUAL_INT16(90, data.oilTempCelsius);
    TEST_ASSERT_EQUAL_UINT8(45, data.oilPressureX10);
}

// Test: Complete workflow - set all values and verify
void test_complete_workflow() {
    // Set all sensor values
    logger->setEGT1(650, true);
    logger->setCHT1(175, false);
    logger->setOilTemp(98, false);
    logger->setEngineAmbient(22, false);
    logger->setOilPressure(48, false);
    logger->setRPM(2800);
    logger->setAlertFlag(ALERT_EGT_HIGH, true);
    logger->setAlertFlag(ALERT_SENSOR_FAILURE, true);

    // Verify all values
    const EngineData1& data = logger->getData();
    TEST_ASSERT_EQUAL_INT16(650, data.egt1Celsius);
    TEST_ASSERT_TRUE(data.egt1OverTemp);
    TEST_ASSERT_EQUAL_INT16(175, data.cht1Celsius);
    TEST_ASSERT_FALSE(data.cht1OverTemp);
    TEST_ASSERT_EQUAL_INT16(98, data.oilTempCelsius);
    TEST_ASSERT_FALSE(data.oilTempOverTemp);
    TEST_ASSERT_EQUAL_INT16(22, data.engineAmbientCelsius);
    TEST_ASSERT_FALSE(data.engineAmbientOverTemp);
    TEST_ASSERT_EQUAL_UINT8(48, data.oilPressureX10);
    TEST_ASSERT_FALSE(data.oilPressureUnder);
    TEST_ASSERT_EQUAL_UINT16(2800, data.rpm);
    TEST_ASSERT_EQUAL_UINT8(ALERT_EGT_HIGH | ALERT_SENSOR_FAILURE, data.alerts);
}

void setup() {
    delay(2000);  // Wait for serial monitor
    UNITY_BEGIN();

    // Constructor and initialization tests
    RUN_TEST(test_constructor_clears_data);
    RUN_TEST(test_get_message_type);

    // EGT1 tests
    RUN_TEST(test_set_egt1_normal);
    RUN_TEST(test_set_egt1_with_warning);
    RUN_TEST(test_set_egt1_boundary_values);

    // CHT1 tests
    RUN_TEST(test_set_cht1_normal);
    RUN_TEST(test_set_cht1_with_warning);

    // Oil temperature tests
    RUN_TEST(test_set_oil_temp_normal);
    RUN_TEST(test_set_oil_temp_with_warning);

    // Engine ambient temperature tests
    RUN_TEST(test_set_engine_ambient_normal);
    RUN_TEST(test_set_engine_ambient_negative);
    RUN_TEST(test_set_engine_ambient_with_warning);

    // Oil pressure tests
    RUN_TEST(test_set_oil_pressure_normal);
    RUN_TEST(test_set_oil_pressure_with_warning);
    RUN_TEST(test_set_oil_pressure_boundary);

    // RPM tests
    RUN_TEST(test_set_rpm_normal);
    RUN_TEST(test_set_rpm_boundary);

    // Alert flag tests
    RUN_TEST(test_set_alert_flag_enable_single);
    RUN_TEST(test_set_alert_flag_enable_multiple);
    RUN_TEST(test_set_alert_flag_disable_single);
    RUN_TEST(test_set_alert_flag_toggle);
    RUN_TEST(test_set_alert_flag_all_flags);

    // Data management tests
    RUN_TEST(test_clear_data);
    RUN_TEST(test_get_data);
    RUN_TEST(test_set_data);
    RUN_TEST(test_set_data_then_modify);

    // Integration tests
    RUN_TEST(test_complete_workflow);

    UNITY_END();
}

void loop() {
    // Nothing here - tests run once in setup()
}
