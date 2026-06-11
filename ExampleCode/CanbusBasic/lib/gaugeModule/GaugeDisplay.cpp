#include <Arduino.h>
#include <EventManager.h>
#include <EEPROM.h>
#include <EepromMap.h>
#include <Stepper.h>
#include "GaugeDisplay.h"

namespace Gauge {

    #define RESPONSE_TIME 200
    #define DEFAULT_SPEED 60

    // Define the default steps and maximum steps for the stepper motor
    const uint16_t stepperSettings[] = {
        // Default steps
        200, 200,
        // X27.168
        // 600 steps => 315 degress
        // 315 degrees => 0.525 degrees per step
        // Steps per revolution = 360 / 0.525 = 685
        685, 600};

    const int AutoGauge::StepsPerRevolution(int stepperType) {
        return stepperSettings[stepperType * 2];
    }

    const int AutoGauge::MaxSteps(int stepperType) {
        return stepperSettings[(stepperType * 2) + 1];
    }

    AutoGauge::AutoGauge(
        int stepperType,
        uint8_t motorPin1, 
        uint8_t motorPin2, 
        uint8_t motorPin3, 
        uint8_t motorPin4)
        : gaugeStepper(StepsPerRevolution(stepperType), motorPin1, motorPin2, motorPin3, motorPin4)
    {                
        Serial.begin(9600);
        maxSteps = MaxSteps(stepperType);
        int stepsPerRevolution = StepsPerRevolution(stepperType);
        gaugeStepper = Stepper(stepsPerRevolution, motorPin1, motorPin2, motorPin3, motorPin4);

        // Initialize motor pins and steps per revolution
        EEPROM.get(EEPROM_GAUGE_ZERO_OFFSET, zeroOffset);
        if(zeroOffset <= 0) {
            zeroOffset = 0;
        }

        EEPROM.get(EEPROM_GAUGE_SPEED, gaugeSpeed);
        if (gaugeSpeed <= 0) {
            gaugeSpeed = DEFAULT_SPEED;
        }
        gaugeStepper.setSpeed(gaugeSpeed);
    }

    bool AutoGauge::begin() {
        // Initialize the stepper motor and set the speed
        /// loop to max and minimum value, then to zero offset
        gaugeStepper.step(maxSteps);
        delay(RESPONSE_TIME);
        gaugeStepper.step(-maxSteps);
        delay(RESPONSE_TIME);
        gaugeStepper.step(zeroOffset);
        delay(RESPONSE_TIME);
        currentValue = 0;
        return true;
    }

    void AutoGauge::setSpeed(int speed) {
        gaugeSpeed = speed;
        gaugeStepper.setSpeed(gaugeSpeed);
    }

    void AutoGauge::setZero(int offset) {
        zeroOffset = offset;
        EEPROM.put(EEPROM_GAUGE_ZERO_OFFSET, zeroOffset);
    }

    bool AutoGauge::errorOccured() {
        return error;
    }

    void AutoGauge::loop() {
        // Update the gauge display in the main loop
        if (millis() - lastTimerEvent >= 500) {
            lastTimerEvent = millis();
            
            // check if new value in range            
            actualValue = targetValue + zeroOffset;
            int moveToValue = actualValue;
            if(moveToValue < 0) {
                moveToValue = 0;
            }
            if(moveToValue > maxSteps) {
                moveToValue = maxSteps;
            }
            int diff = moveToValue - gaugeValue;
            gaugeStepper.step(diff);
            gaugeValue = moveToValue;
            // Move to the target value
            currentValue = targetValue;
        }
    }

    void AutoGauge::setTargetValue(int value) {
        targetValue = value;
    }

    int AutoGauge::getCurrentValue() {
        return currentValue;
    }

    int AutoGauge::getTargetValue() {
        return targetValue;
    }

    int AutoGauge::getSpeed() {
        return gaugeSpeed;
    }
}