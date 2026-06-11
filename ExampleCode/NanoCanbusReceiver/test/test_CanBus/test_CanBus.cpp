#include <unity.h>
#include <CanBusMCP2515.h>
#include <SPI.h>

// Test fixture
CanBusMCP2515* canBus = nullptr;

void setUp() {
    SPI.begin();
    canBus = new CanBusMCP2515();
}

void tearDown() {
    delete canBus;
    canBus = nullptr;
}

// Test: Initialization in loopback mode
void test_init_loopback() {
    CanBusInterface::Result result = canBus->begin(
        CanBusInterface::SPEED_500KBPS,
        CanBusInterface::MODE_LOOPBACK
    );

    TEST_ASSERT_EQUAL(CanBusInterface::OK, result);
    TEST_ASSERT_TRUE(canBus->isReady());
}

// Test: Send and receive message in loopback
void test_loopback_send_receive() {
    canBus->begin(CanBusInterface::SPEED_500KBPS, CanBusInterface::MODE_LOOPBACK);
    canBus->clearRxBuffer();  // Clear any stale messages

    // Prepare test message
    CanBusInterface::Message txMsg;
    txMsg.id = 0x123;
    txMsg.length = 8;
    txMsg.extended = false;
    txMsg.rtr = false;
    for (int i = 0; i < 8; i++) {
        txMsg.data[i] = i + 1;
    }

    // Send message
    CanBusInterface::Result sendResult = canBus->sendMessage(txMsg);
    TEST_ASSERT_EQUAL(CanBusInterface::OK, sendResult);

    // Wait for loopback
    delay(10);

    // Receive message
    CanBusInterface::Message rxMsg;
    CanBusInterface::Result recvResult = canBus->receiveMessage(rxMsg);

    TEST_ASSERT_EQUAL(CanBusInterface::OK, recvResult);
    TEST_ASSERT_EQUAL_UINT32(txMsg.id, rxMsg.id);
    TEST_ASSERT_EQUAL_UINT8(txMsg.length, rxMsg.length);

    // Verify data
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT_EQUAL_UINT8(txMsg.data[i], rxMsg.data[i]);
    }
}

// Test: Multiple messages
void test_multiple_messages() {
    canBus->begin(CanBusInterface::SPEED_500KBPS, CanBusInterface::MODE_LOOPBACK);
    canBus->clearRxBuffer();  // Clear any stale messages

    for (int msgNum = 0; msgNum < 3; msgNum++) {
        CanBusInterface::Message txMsg;
        txMsg.id = 0x100 + msgNum;
        txMsg.length = 4;
        txMsg.extended = false;
        txMsg.rtr = false;
        txMsg.data[0] = msgNum;
        txMsg.data[1] = msgNum + 1;
        txMsg.data[2] = msgNum + 2;
        txMsg.data[3] = msgNum + 3;

        canBus->sendMessage(txMsg);
        delay(10);

        CanBusInterface::Message rxMsg;
        canBus->receiveMessage(rxMsg);

        TEST_ASSERT_EQUAL_UINT32(txMsg.id, rxMsg.id);
        TEST_ASSERT_EQUAL_UINT8(msgNum, rxMsg.data[0]);
    }
}

// Test: Different message lengths
void test_message_lengths() {
    canBus->begin(CanBusInterface::SPEED_500KBPS, CanBusInterface::MODE_LOOPBACK);
    canBus->clearRxBuffer();  // Clear any stale messages

    for (uint8_t len = 0; len <= 8; len++) {
        CanBusInterface::Message txMsg;
        txMsg.id = 0x200 + len;
        txMsg.length = len;
        txMsg.extended = false;
        txMsg.rtr = false;

        for (uint8_t i = 0; i < len; i++) {
            txMsg.data[i] = len * 10 + i;
        }

        canBus->sendMessage(txMsg);
        delay(10);

        CanBusInterface::Message rxMsg;
        canBus->receiveMessage(rxMsg);

        TEST_ASSERT_EQUAL_UINT8(len, rxMsg.length);
        for (uint8_t i = 0; i < len; i++) {
            TEST_ASSERT_EQUAL_UINT8(txMsg.data[i], rxMsg.data[i]);
        }
    }
}

// Test: Message IDs
void test_message_ids() {
    canBus->begin(CanBusInterface::SPEED_500KBPS, CanBusInterface::MODE_LOOPBACK);
    canBus->clearRxBuffer();  // Clear any stale messages

    uint32_t testIds[] = {0x000, 0x001, 0x123, 0x456, 0x7FF};

    for (int i = 0; i < 5; i++) {
        CanBusInterface::Message txMsg;
        txMsg.id = testIds[i];
        txMsg.length = 1;
        txMsg.extended = false;
        txMsg.rtr = false;
        txMsg.data[0] = i;

        canBus->sendMessage(txMsg);
        delay(10);

        CanBusInterface::Message rxMsg;
        canBus->receiveMessage(rxMsg);

        TEST_ASSERT_EQUAL_UINT32(testIds[i], rxMsg.id);
    }
}

void setup() {
    delay(2000);
    UNITY_BEGIN();

    RUN_TEST(test_init_loopback);
    RUN_TEST(test_loopback_send_receive);
    RUN_TEST(test_multiple_messages);
    RUN_TEST(test_message_lengths);
    RUN_TEST(test_message_ids);

    UNITY_END();
}

void loop() {
}
