#pragma once
#include <WString.h>

/// @brief Simple function that returns a string "Hello World from PlatformIO!\n"
/// @return a valid string
const char* getHelloWorld();

/// @brief  Pad a string with spaces on the left
/// @param str a pointer to a string
/// @param minWidth the required minimum with of the string
/// @return a valid string
String left_pad(const String& str, size_t minWidth);

/// @brief  Pad a string with characters
/// @param str a pointer to a string
/// @param minWidth the required minimum with of the string
/// @param pad_char the character to pad with
/// @return a valid string
String left_pad(const String& str, size_t minWidth, char pad_char);