#include <unity.h>
#include <EepromConfigure.h>
#include <CanBusMCP2515.h>
#include <EEPROM.h>
#include <SPI.h>

using namespace EepromConfig;

// Note: Tests use CAN hardware in loopback mode
// CAN bus will be initialized at 500kbps

// Test fixture
CanBusMCP2515* canBus = nullptr;
EepromConfigure* eepromConfig = nullptr;

void setUp() {
    SPI.begin();
    canBus = new CanBusMCP2515();

    // Initialize CAN hardware in loopback mode (500kbps)
    CanBusInterface::Result initResult = canBus->begin(
        CanBusInterface::SPEED_500KBPS,
        CanBusInterface::MODE_LOOPBACK
    );

    // Note: Tests will fail if CAN init fails
    // Run test_can_hardware_init first to diagnose issues

    eepromConfig = new EepromConfigure(*canBus);
}

void tearDown() {
    delete eepromConfig;
    delete canBus;
    eepromConfig = nullptr;
    canBus = nullptr;
}

// Helper function to read CAN message in loopback mode
bool readCanMessage(uint32_t& msgId, uint8_t& len, uint8_t* buf, uint16_t timeout_ms = 100) {
    CanBusInterface::Message msg;
    CanBusInterface::Result result = canBus->receiveMessage(msg, timeout_ms);

    if (result == CanBusInterface::OK) {
        msgId = msg.id;
        len = msg.length;
        memcpy(buf, msg.data, msg.length);
        return true;
    }
    return false;
}

// Helper function to clear any pending CAN messages
void clearCanBuffer() {
    canBus->clearRxBuffer();
}

// Test: CAN hardware initialization diagnostic
void test_can_hardware_init() {
    Serial.println(F("\n=== CAN Hardware Diagnostic ==="));
    Serial.println(F("Testing CAN bus with CanBusMCP2515 wrapper"));

    // Test basic initialization
    Serial.print(F("Initializing CAN in loopback mode... "));

    CanBusMCP2515 testCan;
    CanBusInterface::Result initResult = testCan.begin(
        CanBusInterface::SPEED_500KBPS,
        CanBusInterface::MODE_LOOPBACK
    );

    if (initResult != CanBusInterface::OK) {
        Serial.println(F("FAILED"));
        Serial.println(F("\n*** HARDWARE ISSUE DETECTED ***"));
        Serial.println(F("Possible causes:"));
        Serial.println(F("1. MCP2515 not connected or powered"));
        Serial.println(F("2. Wrong CS pin (using pin 10)"));
        Serial.println(F("3. SPI wiring issue (MOSI/MISO/SCK)"));
        Serial.println(F("4. Incompatible crystal frequency"));
        TEST_FAIL_MESSAGE("CAN hardware not responding");
        return;
    }
    Serial.println(F("SUCCESS!"));

    // Clear any stale messages
    testCan.clearRxBuffer();

    // Try to send a test message
    Serial.print(F("Testing loopback send... "));
    CanBusInterface::Message txMsg;
    txMsg.id = 0x100;
    txMsg.length = 8;
    txMsg.extended = false;
    txMsg.rtr = false;
    for (int i = 0; i < 8; i++) {
        txMsg.data[i] = i + 1;
    }

    CanBusInterface::Result sendResult = testCan.sendMessage(txMsg);
    if (sendResult != CanBusInterface::OK) {
        Serial.println(F("FAILED"));
        TEST_FAIL_MESSAGE("CAN send failed");
        return;
    }
    Serial.println(F("SUCCESS!"));

    // Try to receive the message back
    Serial.print(F("Testing loopback receive... "));
    delay(10);  // Short delay for loopback

    CanBusInterface::Message rxMsg;
    CanBusInterface::Result recvResult = testCan.receiveMessage(rxMsg, 100);

    if (recvResult != CanBusInterface::OK) {
        Serial.println(F("FAILED - No message in buffer"));
        TEST_FAIL_MESSAGE("Loopback not working");
        return;
    }

    // Verify the message
    if (rxMsg.id != txMsg.id || rxMsg.length != txMsg.length) {
        Serial.println(F("FAILED - Message corrupted"));
        TEST_FAIL_MESSAGE("Loopback message mismatch");
        return;
    }

    Serial.println(F("SUCCESS!"));
    Serial.println(F("=== CAN Hardware OK ===\n"));
    TEST_PASS();
}

// Test: Constructor creates uninitialized instance
void test_constructor_not_initialized() {
    TEST_ASSERT_FALSE(eepromConfig->isReady());
}

// Test: begin() initializes the configurator
void test_begin_initializes() {
    TEST_ASSERT_TRUE(eepromConfig->begin());
    TEST_ASSERT_TRUE(eepromConfig->isReady());
}

// Test: isReady returns correct state
void test_is_ready_state() {
    TEST_ASSERT_FALSE(eepromConfig->isReady());

    eepromConfig->begin();
    TEST_ASSERT_TRUE(eepromConfig->isReady());
}

// Test: processMessage returns false when not initialized
void test_process_message_not_initialized() {
    uint8_t data[8] = {0x10, 0x00, 0x42};
    TEST_ASSERT_FALSE(eepromConfig->processMessage(CONFIGURE_SET_BYTE, 3, data));
}

// Test: processMessage returns false for unknown message type
void test_process_message_unknown_type() {
    eepromConfig->begin();

    uint8_t data[8] = {0x10, 0x00};
    TEST_ASSERT_FALSE(eepromConfig->processMessage(0x999, 2, data));
}

// Test: processMessage handles CONFIGURE_SET_BYTE
void test_process_message_set_byte() {
    eepromConfig->begin();

    // Clear EEPROM location first
    EEPROM.update(0x0100, 0x00);

    // Message: Set byte at address 0x0100 to value 0x42
    uint8_t data[8] = {0x00, 0x01, 0x42};  // addr_lo, addr_hi, value

    eepromConfig->processMessage(CONFIGURE_SET_BYTE, 3, data);

    // Verify EEPROM was written
    TEST_ASSERT_EQUAL_UINT8(0x42, EEPROM.read(0x0100));
}

// Test: processMessage handles CONFIGURE_SET_WORD
void test_process_message_set_word() {
    eepromConfig->begin();

    // Clear EEPROM locations first
    EEPROM.update(0x0200, 0x00);
    EEPROM.update(0x0201, 0x00);

    // Message: Set word at address 0x0200 to value 0x1234
    uint8_t data[8] = {0x00, 0x02, 0x34, 0x12};  // addr_lo, addr_hi, val_lo, val_hi

    eepromConfig->processMessage(CONFIGURE_SET_WORD, 4, data);

    // Verify EEPROM was written (little-endian)
    TEST_ASSERT_EQUAL_UINT8(0x34, EEPROM.read(0x0200));
    TEST_ASSERT_EQUAL_UINT8(0x12, EEPROM.read(0x0201));
}

// Test: processMessage handles CONFIGURE_GET_BYTE
void test_process_message_get_byte() {
    eepromConfig->begin();

    // Pre-write a value to EEPROM
    EEPROM.update(0x0150, 0x99);

    // Message: Get byte from address 0x0150
    uint8_t data[8] = {0x50, 0x01};  // addr_lo, addr_hi

    eepromConfig->processMessage(CONFIGURE_GET_BYTE, 2, data);

    // Verify EEPROM value is still there (wasn't corrupted)
    TEST_ASSERT_EQUAL_UINT8(0x99, EEPROM.read(0x0150));
}

// Test: processMessage handles CONFIGURE_GET_WORD
void test_process_message_get_word() {
    eepromConfig->begin();

    // Pre-write a word value to EEPROM (little-endian)
    EEPROM.update(0x0250, 0xAB);
    EEPROM.update(0x0251, 0xCD);

    // Message: Get word from address 0x0250
    uint8_t data[8] = {0x50, 0x02};  // addr_lo, addr_hi

    eepromConfig->processMessage(CONFIGURE_GET_WORD, 2, data);

    // Verify EEPROM values are still there (weren't corrupted)
    TEST_ASSERT_EQUAL_UINT8(0xAB, EEPROM.read(0x0250));
    TEST_ASSERT_EQUAL_UINT8(0xCD, EEPROM.read(0x0251));
}

// Test: processMessage rejects CONFIGURE_SET_BYTE with insufficient data
void test_process_message_set_byte_insufficient_data() {
    eepromConfig->begin();

    uint8_t data[8] = {0x10, 0x00};  // Only 2 bytes, needs 3
    TEST_ASSERT_FALSE(eepromConfig->processMessage(CONFIGURE_SET_BYTE, 2, data));
}

// Test: processMessage rejects CONFIGURE_SET_WORD with insufficient data
void test_process_message_set_word_insufficient_data() {
    eepromConfig->begin();

    uint8_t data[8] = {0x10, 0x00, 0x42};  // Only 3 bytes, needs 4
    TEST_ASSERT_FALSE(eepromConfig->processMessage(CONFIGURE_SET_WORD, 3, data));
}

// Test: processMessage rejects CONFIGURE_GET_BYTE with insufficient data
void test_process_message_get_byte_insufficient_data() {
    eepromConfig->begin();

    uint8_t data[8] = {0x10};  // Only 1 byte, needs 2
    TEST_ASSERT_FALSE(eepromConfig->processMessage(CONFIGURE_GET_BYTE, 1, data));
}

// Test: processMessage rejects CONFIGURE_GET_WORD with insufficient data
void test_process_message_get_word_insufficient_data() {
    eepromConfig->begin();

    uint8_t data[8] = {0x10};  // Only 1 byte, needs 2
    TEST_ASSERT_FALSE(eepromConfig->processMessage(CONFIGURE_GET_WORD, 1, data));
}

// Test: setByte returns error when not initialized
void test_set_byte_not_initialized() {
    ConfigResult result = eepromConfig->setByte(0x0100, 0x42);
    TEST_ASSERT_EQUAL(CONFIG_ERROR_NOT_INITIALIZED, result);
}

// Test: setByte writes to EEPROM
void test_set_byte_writes_to_eeprom() {
    eepromConfig->begin();
    clearCanBuffer();

    // Clear EEPROM location first
    EEPROM.update(0x0180, 0x00);

    // Should succeed in loopback mode
    ConfigResult result = eepromConfig->setByte(0x0180, 0x55);
    TEST_ASSERT_EQUAL(CONFIG_OK, result);

    // Verify EEPROM was written correctly
    TEST_ASSERT_EQUAL_UINT8(0x55, EEPROM.read(0x0180));
}

// Test: setByte uses EEPROM.update (only writes if changed)
void test_set_byte_uses_update() {
    eepromConfig->begin();

    // Write initial value
    EEPROM.update(0x0190, 0xAA);

    // Write same value again
    eepromConfig->setByte(0x0190, 0xAA);

    TEST_ASSERT_EQUAL_UINT8(0xAA, EEPROM.read(0x0190));
}

// Test: setWord returns error when not initialized
void test_set_word_not_initialized() {
    ConfigResult result = eepromConfig->setWord(0x0100, 0x1234);
    TEST_ASSERT_EQUAL(CONFIG_ERROR_NOT_INITIALIZED, result);
}

// Test: setWord writes to EEPROM in little-endian format
void test_set_word_little_endian() {
    eepromConfig->begin();
    clearCanBuffer();

    // Clear EEPROM locations
    EEPROM.update(0x0300, 0x00);
    EEPROM.update(0x0301, 0x00);

    // Should succeed in loopback mode
    ConfigResult result = eepromConfig->setWord(0x0300, 0xABCD);
    TEST_ASSERT_EQUAL(CONFIG_OK, result);

    // Verify little-endian storage in EEPROM
    TEST_ASSERT_EQUAL_UINT8(0xCD, EEPROM.read(0x0300));  // Low byte first
    TEST_ASSERT_EQUAL_UINT8(0xAB, EEPROM.read(0x0301));  // High byte second
}

// Test: getByte returns error when not initialized
void test_get_byte_not_initialized() {
    ConfigResult result = eepromConfig->getByte(0x0100);
    TEST_ASSERT_EQUAL(CONFIG_ERROR_NOT_INITIALIZED, result);
}

// Test: getByte reads from EEPROM
void test_get_byte_reads_from_eeprom() {
    eepromConfig->begin();
    clearCanBuffer();

    // Pre-write a value
    EEPROM.update(0x01A0, 0x77);

    // Should succeed in loopback mode
    ConfigResult result = eepromConfig->getByte(0x01A0);
    TEST_ASSERT_EQUAL(CONFIG_OK, result);

    // Verify EEPROM value wasn't corrupted by the read
    TEST_ASSERT_EQUAL_UINT8(0x77, EEPROM.read(0x01A0));
}

// Test: getWord returns error when not initialized
void test_get_word_not_initialized() {
    ConfigResult result = eepromConfig->getWord(0x0100);
    TEST_ASSERT_EQUAL(CONFIG_ERROR_NOT_INITIALIZED, result);
}

// Test: getWord reads from EEPROM
void test_get_word_reads_from_eeprom() {
    eepromConfig->begin();
    clearCanBuffer();

    // Pre-write a word value (little-endian)
    EEPROM.update(0x0350, 0x34);  // Low byte
    EEPROM.update(0x0351, 0x12);  // High byte

    // Should succeed in loopback mode
    ConfigResult result = eepromConfig->getWord(0x0350);
    TEST_ASSERT_EQUAL(CONFIG_OK, result);

    // Verify EEPROM values weren't corrupted by the read
    TEST_ASSERT_EQUAL_UINT8(0x34, EEPROM.read(0x0350));
    TEST_ASSERT_EQUAL_UINT8(0x12, EEPROM.read(0x0351));
}

// Test: sendByteValue formats and sends message
void test_send_byte_value_format() {
    eepromConfig->begin();
    clearCanBuffer();

    ConfigResult result = eepromConfig->sendByteValue(0x0123, 0xAB);

    TEST_ASSERT_EQUAL(CONFIG_OK, result);

    // Verify message was sent correctly in loopback mode
    uint32_t msgId;
    uint8_t len;
    uint8_t buf[8];

    TEST_ASSERT_TRUE(readCanMessage(msgId, len, buf));
    TEST_ASSERT_EQUAL_UINT32(CONFIGURE_BYTE_VALUE, msgId);
    TEST_ASSERT_EQUAL_UINT8(3, len);
    TEST_ASSERT_EQUAL_UINT8(0x23, buf[0]);  // addr_lo
    TEST_ASSERT_EQUAL_UINT8(0x01, buf[1]);  // addr_hi
    TEST_ASSERT_EQUAL_UINT8(0xAB, buf[2]);  // value
}

// Test: sendWordValue formats and sends message
void test_send_word_value_format() {
    eepromConfig->begin();
    clearCanBuffer();

    ConfigResult result = eepromConfig->sendWordValue(0x0456, 0xDEAD);

    TEST_ASSERT_EQUAL(CONFIG_OK, result);

    // Verify message was sent correctly in loopback mode
    uint32_t msgId;
    uint8_t len;
    uint8_t buf[8];

    TEST_ASSERT_TRUE(readCanMessage(msgId, len, buf));
    TEST_ASSERT_EQUAL_UINT32(CONFIGURE_WORD_VALUE, msgId);
    TEST_ASSERT_EQUAL_UINT8(4, len);
    TEST_ASSERT_EQUAL_UINT8(0x56, buf[0]);  // addr_lo
    TEST_ASSERT_EQUAL_UINT8(0x04, buf[1]);  // addr_hi
    TEST_ASSERT_EQUAL_UINT8(0xAD, buf[2]);  // val_lo
    TEST_ASSERT_EQUAL_UINT8(0xDE, buf[3]);  // val_hi
}

// Test: Complete workflow - set byte, verify EEPROM and CAN
void test_workflow_set_byte() {
    eepromConfig->begin();
    clearCanBuffer();

    // Clear location
    EEPROM.update(0x0400, 0x00);

    // Set a byte value - should succeed in loopback mode
    ConfigResult setResult = eepromConfig->setByte(0x0400, 0x88);
    TEST_ASSERT_EQUAL(CONFIG_OK, setResult);

    // Verify EEPROM was written
    TEST_ASSERT_EQUAL_UINT8(0x88, EEPROM.read(0x0400));

    // Verify CAN message was sent
    uint32_t msgId;
    uint8_t len;
    uint8_t buf[8];
    TEST_ASSERT_TRUE(readCanMessage(msgId, len, buf));
    TEST_ASSERT_EQUAL_UINT32(CONFIGURE_BYTE_VALUE, msgId);
    TEST_ASSERT_EQUAL_UINT8(0x88, buf[2]);  // Value should match

    clearCanBuffer();

    // Get the same byte value
    ConfigResult getResult = eepromConfig->getByte(0x0400);
    TEST_ASSERT_EQUAL(CONFIG_OK, getResult);

    // Verify EEPROM still has correct value
    TEST_ASSERT_EQUAL_UINT8(0x88, EEPROM.read(0x0400));

    // Verify GET response was sent
    TEST_ASSERT_TRUE(readCanMessage(msgId, len, buf));
    TEST_ASSERT_EQUAL_UINT32(CONFIGURE_BYTE_VALUE, msgId);
    TEST_ASSERT_EQUAL_UINT8(0x88, buf[2]);
}

// Test: Complete workflow - set word, verify EEPROM and CAN
void test_workflow_set_word() {
    eepromConfig->begin();
    clearCanBuffer();

    // Clear locations
    EEPROM.update(0x0500, 0x00);
    EEPROM.update(0x0501, 0x00);

    // Set a word value - should succeed in loopback mode
    ConfigResult setResult = eepromConfig->setWord(0x0500, 0xBEEF);
    TEST_ASSERT_EQUAL(CONFIG_OK, setResult);

    // Verify EEPROM was written (little-endian)
    TEST_ASSERT_EQUAL_UINT8(0xEF, EEPROM.read(0x0500));  // Low byte
    TEST_ASSERT_EQUAL_UINT8(0xBE, EEPROM.read(0x0501));  // High byte

    // Verify CAN message was sent
    uint32_t msgId;
    uint8_t len;
    uint8_t buf[8];
    TEST_ASSERT_TRUE(readCanMessage(msgId, len, buf));
    TEST_ASSERT_EQUAL_UINT32(CONFIGURE_WORD_VALUE, msgId);
    TEST_ASSERT_EQUAL_UINT8(0xEF, buf[2]);  // val_lo
    TEST_ASSERT_EQUAL_UINT8(0xBE, buf[3]);  // val_hi

    clearCanBuffer();

    // Get the same word value
    ConfigResult getResult = eepromConfig->getWord(0x0500);
    TEST_ASSERT_EQUAL(CONFIG_OK, getResult);

    // Verify EEPROM still has correct values
    TEST_ASSERT_EQUAL_UINT8(0xEF, EEPROM.read(0x0500));
    TEST_ASSERT_EQUAL_UINT8(0xBE, EEPROM.read(0x0501));

    // Verify GET response was sent
    TEST_ASSERT_TRUE(readCanMessage(msgId, len, buf));
    TEST_ASSERT_EQUAL_UINT32(CONFIGURE_WORD_VALUE, msgId);
    TEST_ASSERT_EQUAL_UINT8(0xEF, buf[2]);  // val_lo
    TEST_ASSERT_EQUAL_UINT8(0xBE, buf[3]);  // val_hi
}

// Test: Message format - SET_BYTE with various addresses
void test_message_format_set_byte() {
    eepromConfig->begin();

    // Test address 0x0000
    EEPROM.update(0x0000, 0x00);
    uint8_t data1[8] = {0x00, 0x00, 0x11};
    eepromConfig->processMessage(CONFIGURE_SET_BYTE, 3, data1);
    TEST_ASSERT_EQUAL_UINT8(0x11, EEPROM.read(0x0000));

    // Test address 0x1234
    uint8_t data2[8] = {0x34, 0x12, 0x22};
    eepromConfig->processMessage(CONFIGURE_SET_BYTE, 3, data2);
    TEST_ASSERT_EQUAL_UINT8(0x22, EEPROM.read(0x1234));
}

// Test: Message format - SET_WORD with various values
void test_message_format_set_word() {
    eepromConfig->begin();

    uint16_t testAddr = 0x0600;

    // Test 0x0000
    uint8_t data1[8] = {0x00, 0x06, 0x00, 0x00};
    eepromConfig->processMessage(CONFIGURE_SET_WORD, 4, data1);
    TEST_ASSERT_EQUAL_UINT8(0x00, EEPROM.read(testAddr));
    TEST_ASSERT_EQUAL_UINT8(0x00, EEPROM.read(testAddr + 1));

    // Test 0xFFFF
    uint8_t data2[8] = {0x00, 0x06, 0xFF, 0xFF};
    eepromConfig->processMessage(CONFIGURE_SET_WORD, 4, data2);
    TEST_ASSERT_EQUAL_UINT8(0xFF, EEPROM.read(testAddr));
    TEST_ASSERT_EQUAL_UINT8(0xFF, EEPROM.read(testAddr + 1));

    // Test 0xABCD (stored as CD AB in little-endian)
    uint8_t data3[8] = {0x00, 0x06, 0xCD, 0xAB};
    eepromConfig->processMessage(CONFIGURE_SET_WORD, 4, data3);
    TEST_ASSERT_EQUAL_UINT8(0xCD, EEPROM.read(testAddr));
    TEST_ASSERT_EQUAL_UINT8(0xAB, EEPROM.read(testAddr + 1));
}

// Test: Byte values - min, max, middle
void test_value_range_byte() {
    eepromConfig->begin();

    uint16_t testAddr = 0x0700;

    // Minimum value (0x00)
    uint8_t data1[8] = {0x00, 0x07, 0x00};
    eepromConfig->processMessage(CONFIGURE_SET_BYTE, 3, data1);
    TEST_ASSERT_EQUAL_UINT8(0x00, EEPROM.read(testAddr));

    // Maximum value (0xFF)
    uint8_t data2[8] = {0x00, 0x07, 0xFF};
    eepromConfig->processMessage(CONFIGURE_SET_BYTE, 3, data2);
    TEST_ASSERT_EQUAL_UINT8(0xFF, EEPROM.read(testAddr));

    // Middle value (0x7F)
    uint8_t data3[8] = {0x00, 0x07, 0x7F};
    eepromConfig->processMessage(CONFIGURE_SET_BYTE, 3, data3);
    TEST_ASSERT_EQUAL_UINT8(0x7F, EEPROM.read(testAddr));
}

void setup() {
    delay(2000);  // Wait for serial monitor
    UNITY_BEGIN();

    // Hardware diagnostic test - run first to verify CAN setup
    RUN_TEST(test_can_hardware_init);

    // Initialization tests
    RUN_TEST(test_constructor_not_initialized);
    RUN_TEST(test_begin_initializes);
    RUN_TEST(test_is_ready_state);

    // processMessage routing tests
    RUN_TEST(test_process_message_not_initialized);
    RUN_TEST(test_process_message_unknown_type);
    RUN_TEST(test_process_message_set_byte);
    RUN_TEST(test_process_message_set_word);
    RUN_TEST(test_process_message_get_byte);
    RUN_TEST(test_process_message_get_word);

    // Message validation tests
    RUN_TEST(test_process_message_set_byte_insufficient_data);
    RUN_TEST(test_process_message_set_word_insufficient_data);
    RUN_TEST(test_process_message_get_byte_insufficient_data);
    RUN_TEST(test_process_message_get_word_insufficient_data);

    // setByte tests
    RUN_TEST(test_set_byte_not_initialized);
    RUN_TEST(test_set_byte_writes_to_eeprom);
    RUN_TEST(test_set_byte_uses_update);

    // setWord tests
    RUN_TEST(test_set_word_not_initialized);
    RUN_TEST(test_set_word_little_endian);

    // getByte tests
    RUN_TEST(test_get_byte_not_initialized);
    RUN_TEST(test_get_byte_reads_from_eeprom);

    // getWord tests
    RUN_TEST(test_get_word_not_initialized);
    RUN_TEST(test_get_word_reads_from_eeprom);

    // Response formatting tests
    RUN_TEST(test_send_byte_value_format);
    RUN_TEST(test_send_word_value_format);

    // Integration workflow tests
    RUN_TEST(test_workflow_set_byte);
    RUN_TEST(test_workflow_set_word);

    // Message format and parsing tests
    RUN_TEST(test_message_format_set_byte);
    RUN_TEST(test_message_format_set_word);
    RUN_TEST(test_value_range_byte);

    UNITY_END();
}

void loop() {
    // Nothing here - tests run once in setup()
}
