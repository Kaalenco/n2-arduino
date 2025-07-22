/**
 * @file controls.h
 * @brief Hardware control classes for buttons, rotary encoders, and keypads
 * 
 * This library provides debounced, event-driven controls for Arduino projects.
 * All controls use the EventManager system for clean, decoupled event handling.
 * 
 * @author N2-Arduino Project
 * @version 1.0
 * @date 2025
 * 
 * @section USAGE Basic Usage Examples
 * 
 * @subsection button_example Button Control Example
 * @code
 * #include "controls/controls.h"
 * 
 * Controls::ButtonControl button(2);  // Pin 2
 * 
 * void handleButtonPress(int event, int param) {
 *     Serial.println("Button pressed on pin: " + String(param));
 * }
 * 
 * void setup() {
 *     button.Begin();
 *     button.eventManager.addListener(EventManager::kEventKeyPress, handleButtonPress);
 * }
 * 
 * void loop() {
 *     button.eventManager.processAllEvents();
 *     button.Loop();
 * }
 * @endcode
 * 
 * @subsection rotary_example Rotary Encoder Example
 * @code
 * #include "controls/controls.h"
 * 
 * Controls::RotaryEncoder rotary(3, 4, 5);  // A=pin3, B=pin4, Button=pin5
 * 
 * void handleRotaryTurn(int event, int param) {
 *     Serial.println("Rotary value: " + String(rotary.RotaryValue()));
 * }
 * 
 * void handleRotaryButton(int event, int param) {
 *     Serial.println("Rotary button pressed");
 * }
 * 
 * void setup() {
 *     rotary.Begin();
 *     rotary.eventManager.addListener(EventManager::kEventMenu0, handleRotaryTurn);
 *     rotary.eventManager.addListener(EventManager::kEventKeyPress, handleRotaryButton);
 * }
 * 
 * void loop() {
 *     rotary.eventManager.processAllEvents();
 *     rotary.Loop();
 * }
 * @endcode
 * 
 * @section BEST_PRACTICES Best Practices
 * 
 * 1. **Always call Begin()** before using any control
 * 2. **Process events every loop cycle** with processAllEvents()
 * 3. **Call Loop() after processing events** to update control state
 * 4. **Use appropriate debounce delays** (default 50ms works for most buttons)
 * 5. **Check error codes** after initialization for debugging
 * 6. **Use event-driven programming** instead of polling ButtonState() directly
 * 
 * @section EVENT_TYPES Event Types
 * 
 * - **kEventKeyPress**: Button pressed down
 * - **kEventKeyRelease**: Button released
 * - **kEventMenu0**: Rotary encoder rotation detected
 * - **kEventUser0-2**: Custom keypad events
 * 
 * @section HARDWARE_SETUP Hardware Setup
 * 
 * @subsection button_hw Button Wiring
 * - Connect button between digital pin and GND
 * - Internal pullup resistors are automatically enabled
 * - No external resistors needed
 * 
 * @subsection rotary_hw Rotary Encoder Wiring
 * - Connect A and B pins to digital inputs
 * - Connect button pin to digital input (optional)
 * - Connect common/GND to Arduino GND
 * - Internal pullups automatically enabled
 */

#pragma once
#include <Arduino.h>
#include <EventManager.h>

using namespace std;

namespace Controls
{
    /**
     * @brief Default debounce delay in milliseconds
     * 
     * This value works well for most mechanical buttons and switches.
     * Increase for noisy switches, decrease for more responsive feel.
     */
    #define DEFAULT_DEBOUNCE_DELAY 50

    /**
     * @class ButtonControl
     * @brief Debounced button control with event management
     * 
     * Provides reliable button input with automatic debouncing and event generation.
     * Can be used standalone or as a base class for more complex controls.
     * 
     * @section button_features Features
     * - Automatic debouncing with configurable delay
     * - Event-driven architecture using EventManager
     * - Press and release event detection
     * - Error handling and diagnostics
     * - Internal pullup resistor support
     * 
     * @section button_events Generated Events
     * - EventManager::kEventKeyPress: When button is pressed
     * - EventManager::kEventKeyRelease: When button is released
     * 
     * @warning Always call Begin() before using the button
     * @warning Call Loop() regularly to update button state
     */
    class ButtonControl {
        protected:
            unsigned long debounceDelay;    ///< Debounce delay in milliseconds
            unsigned long lastDebounceTime; ///< Last time button state changed
            bool lastState;                 ///< Previous button state for edge detection
            int errorCode;                  ///< Error code for diagnostics
            uint8_t pin = UINT8_MAX;       ///< Digital pin number (UINT8_MAX = uninitialized)
            bool buttonState;              ///< Current debounced button state

        public:            
            /**
             * @brief Event manager for button events
             * 
             * Use this to register event listeners and process events.
             * Must call processAllEvents() in your main loop.
             */
            EventManager eventManager;

            /**
             * @brief Initialize the button control
             * 
             * Sets up the pin as INPUT_PULLUP and initializes internal state.
             * Must be called before using the button.
             * 
             * @warning Call this in setup() before using the button
             */
            virtual void Begin();

            /**
             * @brief Update button state and generate events
             * 
             * Reads the button pin, applies debouncing, and generates
             * press/release events. Call this regularly in your main loop.
             * 
             * @return true if button state changed, false otherwise
             * 
             * @note Call processAllEvents() before calling this method
             */
            virtual bool Loop();

            /**
             * @brief Get current debounced button state
             * 
             * @return true if button is currently pressed, false if released
             * 
             * @note Prefer using events over polling this method
             */
            virtual bool ButtonState();

            /**
             * @brief Get the last error code
             * 
             * @return Error code (0 = no error)
             * 
             * @retval 0 No error
             * @retval 1 Invalid pin
             * @retval 2 Initialization failed
             */
            int ErrorCode();

            /**
             * @brief Reset error state and reinitialize
             * 
             * Clears error codes and attempts to reinitialize the control.
             * Useful for error recovery.
             */
            void Reset();

            /**
             * @brief Default constructor
             * 
             * Creates uninitialized button control. Must call constructor
             * with pin number or manually set pin before using.
             */
            ButtonControl();

            /**
             * @brief Constructor with pin number
             * 
             * @param pin Digital pin number for the button
             * 
             * @note Still need to call Begin() before using
             */
            ButtonControl(uint8_t pin);

            /**
             * @brief Constructor with pin and custom debounce delay
             * 
             * @param pin Digital pin number for the button
             * @param debounceDelay Custom debounce delay in milliseconds
             * 
             * @note Use longer delays (100-200ms) for noisy switches
             */
            ButtonControl(uint8_t pin, int debounceDelay);

            /**
             * @brief Destructor
             */
            ~ButtonControl();
    };

    /**
     * @class RotaryEncoder
     * @brief Rotary encoder with optional button, derived from ButtonControl
     * 
     * Handles rotary encoder input with quadrature decoding and optional
     * center button. Inherits all button functionality from ButtonControl.
     * 
     * @section rotary_features Features
     * - Quadrature decoding for reliable rotation detection
     * - Incremental counter with clockwise/counterclockwise detection
     * - Optional center button with debouncing
     * - Event generation for rotation and button presses
     * - All ButtonControl features inherited
     * 
     * @section rotary_events Generated Events
     * - EventManager::kEventMenu0: When rotation is detected
     * - EventManager::kEventKeyPress: When center button is pressed (if connected)
     * - EventManager::kEventKeyRelease: When center button is released (if connected)
     * 
     * @section rotary_usage Usage Tips
     * - Parameter in kEventMenu0 indicates rotation direction (positive = clockwise)
     * - Use RotaryValue() to get absolute position
     * - Center button events use the same system as ButtonControl
     * 
     * @warning Requires two digital pins for A/B signals, plus optional button pin
     */
    class RotaryEncoder : public ButtonControl {
        private:
            bool lastRotaryState;              ///< Previous state of rotary A pin
            unsigned long lastRotaryDebounceTime;  ///< Debounce timer for rotary
            bool lastRotaryDebounceState;      ///< Previous debounced rotary state
            int lastRotaryCounter;             ///< Previous counter value for change detection
            int rotaryCounter;                 ///< Current rotation counter value
            uint8_t rotaryA = UINT8_MAX;      ///< Pin A of rotary encoder
            uint8_t rotaryB = UINT8_MAX;      ///< Pin B of rotary encoder

            /**
             * @brief Internal method to handle encoder logic
             * 
             * Processes quadrature signals and updates rotation counter.
             * Called automatically by Loop().
             * 
             * @return true if rotation detected, false otherwise
             */
            virtual bool LoopEncoder();

        public:
            /**
             * @brief Initialize the rotary encoder
             * 
             * Sets up A/B pins and optional button pin with pullups.
             * Overrides ButtonControl::Begin() to handle encoder pins.
             */
            virtual void Begin();

            /**
             * @brief Update encoder and button state
             * 
             * Processes both rotary encoder signals and button state.
             * Generates events for rotation and button presses.
             * 
             * @return true if any state changed, false otherwise
             */
            virtual bool Loop();

            /**
             * @brief Get current rotation counter value
             * 
             * @return Absolute rotation count (positive = clockwise from start)
             * 
             * @note Counter increments/decrements with each detent
             * @note Use event system for rotation detection rather than polling
             */
            int RotaryValue();

            /**
             * @brief Default constructor
             */
            RotaryEncoder();

            /**
             * @brief Constructor with A and B pins only
             * 
             * @param pinA Digital pin for encoder signal A
             * @param pinB Digital pin for encoder signal B
             * 
             * @note No center button functionality with this constructor
             */
            RotaryEncoder(uint8_t pinA, uint8_t pinB);

            /**
             * @brief Constructor with A, B, and button pins
             * 
             * @param pinA Digital pin for encoder signal A
             * @param pinB Digital pin for encoder signal B
             * @param pinKey Digital pin for center button (optional)
             * 
             * @note This is the most common configuration
             */
            RotaryEncoder(uint8_t pinA, uint8_t pinB, uint8_t pinKey);

            /**
             * @brief Constructor with all pins and custom debounce
             * 
             * @param pinA Digital pin for encoder signal A
             * @param pinB Digital pin for encoder signal B
             * @param pinKey Digital pin for center button
             * @param debounceDelay Custom debounce delay in milliseconds
             */
            RotaryEncoder(uint8_t pinA, uint8_t pinB, uint8_t pinKey, int debounceDelay);

            /**
             * @brief Destructor
             */
            ~RotaryEncoder();
    };

    /**
     * @class KeypadControl
     * @brief Simple keypad/matrix keyboard control
     * 
     * Handles keypad input with debouncing and character buffering.
     * Suitable for simple keypads and matrix keyboards.
     * 
     * @section keypad_features Features
     * - Character-based input with buffering
     * - Automatic debouncing
     * - Key availability checking
     * - Peek functionality (read without consuming)
     * 
     * @warning This is a basic implementation - complex matrix keypads
     *          may require custom scanning logic
     */
    class KeypadControl {
        public:
            /**
             * @brief Constructor with pin only
             * 
             * @param pin Digital pin connected to keypad
             */
            KeypadControl(uint8_t pin);

            /**
             * @brief Constructor with pin and custom debounce
             * 
             * @param pin Digital pin connected to keypad
             * @param debounceDelay Custom debounce delay in milliseconds
             */
            KeypadControl(uint8_t pin, int debounceDelay);

            /**
             * @brief Destructor
             */
            ~KeypadControl();

            /**
             * @brief Initialize keypad control
             */
            void Begin();

            /**
             * @brief Update keypad state
             * 
             * @return true if new key detected, false otherwise
             */
            bool Loop();

            /**
             * @brief Check if key is available to read
             * 
             * @return true if key is buffered and ready, false otherwise
             */
            bool KeyAvailable();

            /**
             * @brief Peek at next key without consuming it
             * 
             * @return Character code of next key, 0 if none available
             */
            char PeekKey();

            /**
             * @brief Read and consume next key
             * 
             * @return Character code of key, 0 if none available
             */
            char ReadKey();

            /**
             * @brief Get last error code
             * 
             * @return Error code (0 = no error)
             */
            int ErrorCode();

            /**
             * @brief Reset error state
             */
            void Reset();

        private:
            unsigned long debounceDelay;    ///< Debounce delay in milliseconds
            unsigned long lastDebounceTime; ///< Last debounce time
            bool lastState;                 ///< Previous key state
            int errorCode;                  ///< Error code for diagnostics
            uint8_t pin;                   ///< Digital pin number
            char key;                      ///< Current key character
    };
}