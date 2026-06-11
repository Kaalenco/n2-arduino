#include <Arduino.h>
#include "coreFunctions.h"

float ToFloat(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3)
{
  float fValue = 0.0;
  uint8_t* p = (uint8_t*)&fValue;
  p[0] = b0;
  p[1] = b1;
  p[2] = b2;
  p[3] = b3;
  return fValue;
}

float ToInt(uint8_t b0, uint8_t b1)
{
  int iValue = 0;
  uint8_t* p = (uint8_t*)&iValue;
  p[0] = b0;
  p[1] = b1;
  return iValue;
}