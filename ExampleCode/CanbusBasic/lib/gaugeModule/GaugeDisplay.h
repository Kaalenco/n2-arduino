#pragma once
#include <EventManager.h>
#include <Stepper.h>

// The GaugeDisplay class is used to display sensor data on a gauge
// using a stepper motor (x27.168)

namespace Gauge {

    // Using a automotive stepper motor to display a value
    class AutoGauge {
        public:
            AutoGauge(
                int stepperType,
                uint8_t motorPin1, 
                uint8_t motorPin2, 
                uint8_t motorPin3, 
                uint8_t motorPin4);

            // Initialize the gauge
            // and begin displaying values
            bool begin();

            // Set the zero offset for the gauge
            void setZero(int offset);
            bool errorOccured();
            void loop();

            void setTargetValue(int value);
            int getCurrentValue();
            int getTargetValue();
            void setSpeed(int speed);
            int getSpeed();
                
            const int StepsPerRevolution(int stepperType);
            const int MaxSteps(int stepperType);
            EventManager eventManager;
        private:
            Stepper gaugeStepper;
            // The target value for the gauge
            int targetValue = 0;
            // The current value of the gauge
            int currentValue = 0;
            // The actual value within the current range
            int actualValue = 0;
            int gaugeValue = 0;
            // The maximum number of steps for the gauge
            int maxSteps = 0;
            // The zero offset for the gauge
            int zeroOffset = 0;
            // The speed of the gauge
            int gaugeSpeed = 0;
            unsigned long lastTimerEvent = 0;
            bool initFailed = false;
            bool error = false;
    };

    // The number of steps per revolution for a stepper motor
    enum StepperType{
        DefaultSteps,
        X27_168
    };
}