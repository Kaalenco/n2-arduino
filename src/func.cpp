#include "func.h"

const char* getHelloWorld()
{
    return "Hello World from PlatformIO!\n";
}

String left_pad(const String& str, size_t minWidth) {
  return left_pad(str, minWidth, ' ');
}

String left_pad(const String& str, size_t minWidth, char pad_char) {
  int slen = minWidth - str.length();
  int filSize = ((slen < 0) ? 0 : slen);
  char fill[filSize+1];
  for(int i = 0; i < filSize; i++){
    fill[i] = pad_char;
  }
  fill[filSize] = '\0';
  return fill + str;
}