#include "unity.h"
#include <Arduino.h>
#include <stdio.h>
#include "../src/func.cpp"

void setUp(void) {
  // set stuff up here
}

void tearDown(void) {
  // clean stuff up here
}

void test_function_should_do(void) {
  const char* result = getHelloWorld();
  TEST_ASSERT_EQUAL_STRING("Hello World from PlatformIO!\n", result);
}

void test_function_should_do_too(void) {
  // more test stuff
}

void pad_string_with_spaces(void) {
  String result = left_pad("test", 10);
  TEST_ASSERT_EQUAL_STRING("      test", result.c_str());
}

void pad_string_with_character(void) {
  String result = left_pad("test", 8, '*');
  TEST_ASSERT_EQUAL_STRING("****test", result.c_str());
}

int runUnityTests(void) {
  UNITY_BEGIN();
  // Sample test
  RUN_TEST(test_function_should_do);
  RUN_TEST(test_function_should_do_too);

  // Test padding
  RUN_TEST(pad_string_with_spaces);
  RUN_TEST(pad_string_with_character);

  return UNITY_END();
}

/**
  * For Arduino framework
  */
void setup() {
  // Wait ~2 seconds before the Unity test runner
  // establishes connection with a board Serial interface
  delay(2000);

  runUnityTests();
}
void loop() {}
