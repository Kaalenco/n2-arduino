#include <unity.h>
#include <EepromConfigure.h>
#include <EEPROM.h>

using namespace EepromConfig;

// Mock MCP_CAN for testing (minimal implementation)
// Since we can't test actual CAN communication without hardware,
// we'll focus on testing the logic and EEPROM operations
class MockMCP_CAN : public MCP_CAN {
public:
    MockMCP_CAN(uint8_t pin) : MCP_CAN(pin), lastSendResult(CAN_OK), sendCallCount(0) {}

    // Override sendMsgBuf to capture calls and return controlled results
    uint8_t sendMsgBuf(uint32_t id, uint8_t ext, uint8_t len, uint8_t *buf) {
        lastMessageId = id;
        lastMessageLen = len;
        memcpy(lastMessageData, buf, len);
        sendCallCount++;
        return lastSendResult;
    }

    // Test control variables
    uint8_t lastSendResult;
    uint32_t lastMessageId;
    uint8_t lastMessageLen;
    uint8_t lastMessageData[8];
    int sendCallCount;

    void resetMock() {
        lastSendResult = CAN_OK;
        lastMessageId = 0;
        lastMessageLen = 0;
        sendCallCount = 0;
        memset(lastMessageData, 0, 8);
    }
};

// Test fixture
MockMCP_CAN* mockCan = nullptr;
EepromConfigure* eepromConfig = nullptr;

void setUp() {
    mockCan = new MockMCP_CAN(10);
    eepromConfig = new EepromConfigure(*mockCan);
}

void tearDown() {
    delete eepromConfig;
    delete mockCan;
    eepromConfig = nullptr;
    mockCan = nullptr;
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
    mockCan->resetMock();

    // Message: Set byte at address 0x0100 to value 0x42
    uint8_t data[8] = {0x00, 0x01, 0x42};  // addr_lo, addr_hi, value
    TEST_ASSERT_TRUE(eepromConfig->processMessage(CONFIGURE_SET_BYTE, 3, data));

    // Verify CAN response was sent
    TEST_ASSERT_EQUAL(1, mockCan->sendCallCount);
    TEST_ASSERT_EQUAL_UINT32(CONFIGURE_BYTE_VALUE, mockCan->lastMessageId);
    TEST_ASSERT_EQUAL_UINT8(3, mockCan->lastMessageLen);

    // Verify response data [addr_lo, addr_hi, value]
    TEST_ASSERT_EQUAL_UINT8(0x00, mockCan->lastMessageData[0]);
    TEST_ASSERT_EQUAL_UINT8(0x01, mockCan->lastMessageData[1]);
    TEST_ASSERT_EQUAL_UINT8(0x42, mockCan->lastMessageData[2]);

    // Verify EEPROM was written
    TEST_ASSERT_EQUAL_UINT8(0x42, EEPROM.read(0x0100));
}

// Test: processMessage handles CONFIGURE_SET_WORD
void test_process_message_set_word() {
    eepromConfig->begin();
    mockCan->resetMock();

    // Message: Set word at address 0x0200 to value 0x1234
    uint8_t data[8] = {0x00, 0x02, 0x34, 0x12};  // addr_lo, addr_hi, val_lo, val_hi
    TEST_ASSERT_TRUE(eepromConfig->processMessage(CONFIGURE_SET_WORD, 4, data));

    // Verify CAN response was sent
    TEST_ASSERT_EQUAL(1, mockCan->sendCallCount);
    TEST_ASSERT_EQUAL_UINT32(CONFIGURE_WORD_VALUE, mockCan->lastMessageId);
    TEST_ASSERT_EQUAL_UINT8(4, mockCan->lastMessageLen);

    // Verify response data [addr_lo, addr_hi, val_lo, val_hi]
    TEST_ASSERT_EQUAL_UINT8(0x00, mockCan->lastMessageData[0]);
    TEST_ASSERT_EQUAL_UINT8(0x02, mockCan->lastMessageData[1]);
    TEST_ASSERT_EQUAL_UINT8(0x34, mockCan->lastMessageData[2]);
    TEST_ASSERT_EQUAL_UINT8(0x12, mockCan->lastMessageData[3]);

    // Verify EEPROM was written (little-endian)
    TEST_ASSERT_EQUAL_UINT8(0x34, EEPROM.read(0x0200));
    TEST_ASSERT_EQUAL_UINT8(0x12, EEPROM.read(0x0201));
}

// Test: processMessage handles CONFIGURE_GET_BYTE
void test_process_message_get_byte() {
    eepromConfig->begin();

    // Pre-write a value to EEPROM
    EEPROM.update(0x0150, 0x99);

    mockCan->resetMock();

    // Message: Get byte from address 0x0150
    uint8_t data[8] = {0x50, 0x01};  // addr_lo, addr_hi
    TEST_ASSERT_TRUE(eepromConfig->processMessage(CONFIGURE_GET_BYTE, 2, data));

    // Verify CAN response was sent
    TEST_ASSERT_EQUAL(1, mockCan->sendCallCount);
    TEST_ASSERT_EQUAL_UINT32(CONFIGURE_BYTE_VALUE, mockCan->lastMessageId);

    // Verify response contains the read value
    TEST_ASSERT_EQUAL_UINT8(0x50, mockCan->lastMessageData[0]);
    TEST_ASSERT_EQUAL_UINT8(0x01, mockCan->lastMessageData[1]);
    TEST_ASSERT_EQUAL_UINT8(0x99, mockCan->lastMessageData[2]);
}

// Test: processMessage handles CONFIGURE_GET_WORD
void test_process_message_get_word() {
    eepromConfig->begin();

    // Pre-write a word value to EEPROM (little-endian)
    EEPROM.update(0x0250, 0xAB);
    EEPROM.update(0x0251, 0xCD);

    mockCan->resetMock();

    // Message: Get word from address 0x0250
    uint8_t data[8] = {0x50, 0x02};  // addr_lo, addr_hi
    TEST_ASSERT_TRUE(eepromConfig->processMessage(CONFIGURE_GET_WORD, 2, data));

    // Verify CAN response was sent
    TEST_ASSERT_EQUAL(1, mockCan->sendCallCount);
    TEST_ASSERT_EQUAL_UINT32(CONFIGURE_WORD_VALUE, mockCan->lastMessageId);

    // Verify response contains the read value
    TEST_ASSERT_EQUAL_UINT8(0x50, mockCan->lastMessageData[0]);
    TEST_ASSERT_EQUAL_UINT8(0x02, mockCan->lastMessageData[1]);
    TEST_ASSERT_EQUAL_UINT8(0xAB, mockCan->lastMessageData[2]);
    TEST_ASSERT_EQUAL_UINT8(0xCD, mockCan->lastMessageData[3]);
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

// Test: setByte writes to EEPROM and sends response
void test_set_byte_writes_and_responds() {
    eepromConfig->begin();
    mockCan->resetMock();

    ConfigResult result = eepromConfig->setByte(0x0180, 0x55);

    TEST_ASSERT_EQUAL(CONFIG_OK, result);
    TEST_ASSERT_EQUAL_UINT8(0x55, EEPROM.read(0x0180));
    TEST_ASSERT_EQUAL(1, mockCan->sendCallCount);
}

// Test: setByte uses EEPROM.update (only writes if changed)
void test_set_byte_uses_update() {
    eepromConfig->begin();

    // Write initial value
    EEPROM.update(0x0190, 0xAA);

    // Write same value again
    eepromConfig->setByte(0x0190, 0xAA);

    // EEPROM.update should not rewrite if value is same
    // (We can't directly test this without EEPROM wear tracking,
    // but we verify the value is correct)
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
    mockCan->resetMock();

    ConfigResult result = eepromConfig->setWord(0x0300, 0xABCD);

    TEST_ASSERT_EQUAL(CONFIG_OK, result);

    // Verify little-endian storage
    TEST_ASSERT_EQUAL_UINT8(0xCD, EEPROM.read(0x0300));  // Low byte first
    TEST_ASSERT_EQUAL_UINT8(0xAB, EEPROM.read(0x0301));  // High byte second

    TEST_ASSERT_EQUAL(1, mockCan->sendCallCount);
}

// Test: getByte returns error when not initialized
void test_get_byte_not_initialized() {
    ConfigResult result = eepromConfig->getByte(0x0100);
    TEST_ASSERT_EQUAL(CONFIG_ERROR_NOT_INITIALIZED, result);
}

// Test: getByte reads from EEPROM and sends response
void test_get_byte_reads_and_responds() {
    eepromConfig->begin();

    // Pre-write a value
    EEPROM.update(0x01A0, 0x77);

    mockCan->resetMock();
    ConfigResult result = eepromConfig->getByte(0x01A0);

    TEST_ASSERT_EQUAL(CONFIG_OK, result);
    TEST_ASSERT_EQUAL(1, mockCan->sendCallCount);
    TEST_ASSERT_EQUAL_UINT8(0x77, mockCan->lastMessageData[2]);
}

// Test: getWord returns error when not initialized
void test_get_word_not_initialized() {
    ConfigResult result = eepromConfig->getWord(0x0100);
    TEST_ASSERT_EQUAL(CONFIG_ERROR_NOT_INITIALIZED, result);
}

// Test: getWord reads from EEPROM in little-endian format
void test_get_word_little_endian() {
    eepromConfig->begin();

    // Pre-write a word value (little-endian)
    EEPROM.update(0x0350, 0x34);  // Low byte
    EEPROM.update(0x0351, 0x12);  // High byte

    mockCan->resetMock();
    ConfigResult result = eepromConfig->getWord(0x0350);

    TEST_ASSERT_EQUAL(CONFIG_OK, result);
    TEST_ASSERT_EQUAL(1, mockCan->sendCallCount);

    // Verify response contains correct little-endian bytes
    TEST_ASSERT_EQUAL_UINT8(0x34, mockCan->lastMessageData[2]);  // Low byte
    TEST_ASSERT_EQUAL_UINT8(0x12, mockCan->lastMessageData[3]);  // High byte
}

// Test: sendByteValue formats message correctly
void test_send_byte_value_format() {
    eepromConfig->begin();
    mockCan->resetMock();

    ConfigResult result = eepromConfig->sendByteValue(0x0123, 0xAB);

    TEST_ASSERT_EQUAL(CONFIG_OK, result);
    TEST_ASSERT_EQUAL_UINT32(CONFIGURE_BYTE_VALUE, mockCan->lastMessageId);
    TEST_ASSERT_EQUAL_UINT8(3, mockCan->lastMessageLen);
    TEST_ASSERT_EQUAL_UINT8(0x23, mockCan->lastMessageData[0]);  // addr_lo
    TEST_ASSERT_EQUAL_UINT8(0x01, mockCan->lastMessageData[1]);  // addr_hi
    TEST_ASSERT_EQUAL_UINT8(0xAB, mockCan->lastMessageData[2]);  // value
}

// Test: sendWordValue formats message correctly
void test_send_word_value_format() {
    eepromConfig->begin();
    mockCan->resetMock();

    ConfigResult result = eepromConfig->sendWordValue(0x0456, 0xDEAD);

    TEST_ASSERT_EQUAL(CONFIG_OK, result);
    TEST_ASSERT_EQUAL_UINT32(CONFIGURE_WORD_VALUE, mockCan->lastMessageId);
    TEST_ASSERT_EQUAL_UINT8(4, mockCan->lastMessageLen);
    TEST_ASSERT_EQUAL_UINT8(0x56, mockCan->lastMessageData[0]);  // addr_lo
    TEST_ASSERT_EQUAL_UINT8(0x04, mockCan->lastMessageData[1]);  // addr_hi
    TEST_ASSERT_EQUAL_UINT8(0xAD, mockCan->lastMessageData[2]);  // val_lo
    TEST_ASSERT_EQUAL_UINT8(0xDE, mockCan->lastMessageData[3]);  // val_hi
}

// Test: sendByteValue returns error on CAN send failure
void test_send_byte_value_can_failure() {
    eepromConfig->begin();
    mockCan->resetMock();
    mockCan->lastSendResult = CAN_SENDMSGTIMEOUT;

    ConfigResult result = eepromConfig->sendByteValue(0x0100, 0x42);

    TEST_ASSERT_EQUAL(CONFIG_ERROR_SEND_FAILED, result);
}

// Test: sendWordValue returns error on CAN send failure
void test_send_word_value_can_failure() {
    eepromConfig->begin();
    mockCan->resetMock();
    mockCan->lastSendResult = CAN_SENDMSGTIMEOUT;

    ConfigResult result = eepromConfig->sendWordValue(0x0100, 0x1234);

    TEST_ASSERT_EQUAL(CONFIG_ERROR_SEND_FAILED, result);
}

// Test: Address encoding/decoding for 16-bit addresses
void test_address_encoding_boundary_values() {
    eepromConfig->begin();
    mockCan->resetMock();

    // Test minimum address (0x0000)
    eepromConfig->sendByteValue(0x0000, 0xFF);
    TEST_ASSERT_EQUAL_UINT8(0x00, mockCan->lastMessageData[0]);
    TEST_ASSERT_EQUAL_UINT8(0x00, mockCan->lastMessageData[1]);

    // Test maximum address (0xFFFF)
    eepromConfig->sendByteValue(0xFFFF, 0xFF);
    TEST_ASSERT_EQUAL_UINT8(0xFF, mockCan->lastMessageData[0]);
    TEST_ASSERT_EQUAL_UINT8(0xFF, mockCan->lastMessageData[1]);

    // Test middle address (0x1234)
    eepromConfig->sendByteValue(0x1234, 0xFF);
    TEST_ASSERT_EQUAL_UINT8(0x34, mockCan->lastMessageData[0]);
    TEST_ASSERT_EQUAL_UINT8(0x12, mockCan->lastMessageData[1]);
}

// Test: Complete workflow - set byte, get byte
void test_workflow_set_get_byte() {
    eepromConfig->begin();

    // Set a byte value
    ConfigResult setResult = eepromConfig->setByte(0x0400, 0x88);
    TEST_ASSERT_EQUAL(CONFIG_OK, setResult);

    // Get the same byte value
    mockCan->resetMock();
    ConfigResult getResult = eepromConfig->getByte(0x0400);
    TEST_ASSERT_EQUAL(CONFIG_OK, getResult);

    // Verify the response contains the value we set
    TEST_ASSERT_EQUAL_UINT8(0x88, mockCan->lastMessageData[2]);
}

// Test: Complete workflow - set word, get word
void test_workflow_set_get_word() {
    eepromConfig->begin();

    // Set a word value
    ConfigResult setResult = eepromConfig->setWord(0x0500, 0xBEEF);
    TEST_ASSERT_EQUAL(CONFIG_OK, setResult);

    // Get the same word value
    mockCan->resetMock();
    ConfigResult getResult = eepromConfig->getWord(0x0500);
    TEST_ASSERT_EQUAL(CONFIG_OK, getResult);

    // Verify the response contains the value we set (little-endian)
    TEST_ASSERT_EQUAL_UINT8(0xEF, mockCan->lastMessageData[2]);  // Low byte
    TEST_ASSERT_EQUAL_UINT8(0xBE, mockCan->lastMessageData[3]);  // High byte
}

void setup() {
    delay(2000);  // Wait for serial monitor
    UNITY_BEGIN();

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
    RUN_TEST(test_set_byte_writes_and_responds);
    RUN_TEST(test_set_byte_uses_update);

    // setWord tests
    RUN_TEST(test_set_word_not_initialized);
    RUN_TEST(test_set_word_little_endian);

    // getByte tests
    RUN_TEST(test_get_byte_not_initialized);
    RUN_TEST(test_get_byte_reads_and_responds);

    // getWord tests
    RUN_TEST(test_get_word_not_initialized);
    RUN_TEST(test_get_word_little_endian);

    // Response formatting tests
    RUN_TEST(test_send_byte_value_format);
    RUN_TEST(test_send_word_value_format);

    // Error handling tests
    RUN_TEST(test_send_byte_value_can_failure);
    RUN_TEST(test_send_word_value_can_failure);

    // Address encoding tests
    RUN_TEST(test_address_encoding_boundary_values);

    // Integration workflow tests
    RUN_TEST(test_workflow_set_get_byte);
    RUN_TEST(test_workflow_set_get_word);

    UNITY_END();
}

void loop() {
    // Nothing here - tests run once in setup()
}
