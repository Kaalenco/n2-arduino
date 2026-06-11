# PlatformIO Unit Testing

## Test Directory Structure

**CRITICAL**: Each test suite MUST be in its own subdirectory under `test/`.

### Correct Structure
```
test/
├── test_EngineData1Packer/
│   └── test_EngineData1Packer.cpp
├── test_EngineDataLogger/
│   └── test_EngineDataLogger.cpp
└── test_SensorReader/
    └── test_SensorReader.cpp
```

### Incorrect Structure (causes multiple definition errors)
```
test/
├── test_EngineData1Packer.cpp  ❌ Don't put test files directly in test/
└── test_EngineDataLogger.cpp   ❌ Will cause linker errors
```

## Why This Matters

PlatformIO compiles each test directory separately. If multiple test files are in the same directory, they will be compiled together, causing multiple definition errors for `setup()` and `loop()` functions.

**Error Example:**
```
multiple definition of `setup'
multiple definition of `loop'
```

## Test File Pattern

Each test file follows the Unity framework pattern:

```cpp
#include <unity.h>
#include <YourClass.h>

// Test fixtures (optional)
YourClass* instance = nullptr;

void setUp() {
    instance = new YourClass();
}

void tearDown() {
    delete instance;
    instance = nullptr;
}

// Test functions
void test_something() {
    TEST_ASSERT_EQUAL(expected, actual);
}

void setup() {
    delay(2000);  // Wait for serial monitor
    UNITY_BEGIN();
    
    RUN_TEST(test_something);
    
    UNITY_END();
}

void loop() {
    // Empty - tests run once in setup()
}
```

## Running Tests

```bash
# Run all tests
pio test

# Run specific test by folder name
pio test -f test_EngineDataLogger

# Run with verbose output
pio test -vv

# Run with very verbose output (for debugging)
pio test -vvv
```

## Common Unity Assertions

```cpp
TEST_ASSERT_TRUE(condition)
TEST_ASSERT_FALSE(condition)
TEST_ASSERT_EQUAL(expected, actual)
TEST_ASSERT_EQUAL_INT16(expected, actual)
TEST_ASSERT_EQUAL_UINT8(expected, actual)
TEST_ASSERT_EQUAL_UINT16(expected, actual)
TEST_ASSERT_NOT_EQUAL(expected, actual)
TEST_ASSERT_NULL(pointer)
TEST_ASSERT_NOT_NULL(pointer)
```

## Testing Without Hardware

For classes that depend on hardware (like MCP_CAN for CAN bus), focus tests on:
- Data management methods (setters/getters)
- Data structure manipulation
- Validation logic
- State management

Hardware-dependent methods like `begin()` and `sendFrame()` may not be testable in native environment without mocking.

## Project-Specific Notes

- Tests use hardware-specific types (int16_t, uint8_t, etc.) from Arduino.h
- Allow tolerance for quantized values (e.g., temperatures with step sizes)
- Test boundary conditions (min/max values, clamping)
- Test flag manipulation (bitwise operations)
