#ifndef RPM_SENSOR_H
#define RPM_SENSOR_H

#include <Arduino.h>

// Interrupt-driven RPM measurement.
// An ISR counts falling edges from a magneto or ignition pickup.
// Call readRpm() at a fixed interval and pass the actual elapsed time in ms.
//
// Include this header in exactly one translation unit (main.cpp).
// The static ISR and pulse counter are local to that translation unit.

namespace RpmSensor {

namespace Internal {
    static volatile uint16_t pulseCount = 0;
    static uint8_t pulsesPerRev = 1;

    static void pulseIsr() {
        pulseCount++;
    }
}

static void begin(uint8_t pin, uint8_t pulsesPerRev, uint8_t triggerMode = FALLING) {
    Internal::pulsesPerRev = (pulsesPerRev == 0) ? 1 : pulsesPerRev;
    pinMode(pin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(pin), Internal::pulseIsr, triggerMode);
}

// Read the accumulated pulse count, reset it, and return the calculated RPM.
// elapsedMs is the actual time since the previous call, in milliseconds.
static uint16_t readRpm(unsigned long elapsedMs) {
    noInterrupts();
    uint16_t count = Internal::pulseCount;
    Internal::pulseCount = 0;
    interrupts();

    if (count == 0 || elapsedMs == 0) return 0;
    return (uint32_t)count * 60000UL / ((uint32_t)Internal::pulsesPerRev * elapsedMs);
}

} // namespace RpmSensor

#endif // RPM_SENSOR_H
