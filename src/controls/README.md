# Controls Library Documentation

## Overview

The Controls library provides robust, event-driven input controls for Arduino projects. It includes debounced buttons, rotary encoders, and keypad support with integrated event management.

## Quick Start

### Basic Button Setup

```cpp
#include "controls/controls.h"

Controls::ButtonControl button(2);  // Connect button to pin 2

void handleButtonPress(int event, int param) {
    Serial.println("Button pressed!");
}

void handleButtonRelease(int event, int param) {
    Serial.println("Button released!");
}

void setup() {
    Serial.begin(9600);
    button.Begin();
    
    // Register event handlers
    button.eventManager.addListener(EventManager::kEventKeyPress, handleButtonPress);
    button.eventManager.addListener(EventManager::kEventKeyRelease, handleButtonRelease);
}

void loop() {
    // Process events first, then update controls
    button.eventManager.processAllEvents();
    button.Loop();
    
    delay(10);  // Small delay for stability
}
```

### Rotary Encoder with Button

```cpp
#include "controls/controls.h"

Controls::RotaryEncoder rotary(3, 4, 5);  // A=3, B=4, Button=5
int menuValue = 0;

void handleRotaryTurn(int event, int param) {
    menuValue = rotary.RotaryValue();
    Serial.println("Menu position: " + String(menuValue));
}

void handleRotaryPress(int event, int param) {
    Serial.println("Selected: " + String(menuValue));
}

void setup() {
    Serial.begin(9600);
    rotary.Begin();
    
    rotary.eventManager.addListener(EventManager::kEventMenu0, handleRotaryTurn);
    rotary.eventManager.addListener(EventManager::kEventKeyPress, handleRotaryPress);
}

void loop() {
    rotary.eventManager.processAllEvents();
    rotary.Loop();
    delay(10);
}
```

## Advanced Patterns

### Multi-Control System

```cpp
#include "controls/controls.h"

Controls::ButtonControl upButton(2);
Controls::ButtonControl downButton(3);
Controls::ButtonControl selectButton(4);
Controls::RotaryEncoder rotary(5, 6, 7);

class MenuSystem {
private:
    int currentMenu = 0;
    int maxMenus = 5;
    
public:
    void init() {
        upButton.Begin();
        downButton.Begin();
        selectButton.Begin();
        rotary.Begin();
        
        // Register all event handlers
        upButton.eventManager.addListener(EventManager::kEventKeyPress, 
            [](int event, int param) { menuSystem.navigateUp(); });
        downButton.eventManager.addListener(EventManager::kEventKeyPress,
            [](int event, int param) { menuSystem.navigateDown(); });
        selectButton.eventManager.addListener(EventManager::kEventKeyPress,
            [](int event, int param) { menuSystem.select(); });
        rotary.eventManager.addListener(EventManager::kEventMenu0,
            [](int event, int param) { menuSystem.handleRotary(); });
    }
    
    void update() {
        upButton.eventManager.processAllEvents();
        downButton.eventManager.processAllEvents();
        selectButton.eventManager.processAllEvents();
        rotary.eventManager.processAllEvents();
        
        upButton.Loop();
        downButton.Loop();
        selectButton.Loop();
        rotary.Loop();
    }
    
    void navigateUp() {
        currentMenu = max(0, currentMenu - 1);
        displayMenu();
    }
    
    void navigateDown() {
        currentMenu = min(maxMenus - 1, currentMenu + 1);
        displayMenu();
    }
    
    void handleRotary() {
        int rotaryVal = rotary.RotaryValue();
        currentMenu = constrain(rotaryVal, 0, maxMenus - 1);
        displayMenu();
    }
    
    void select() {
        Serial.println("Selected menu: " + String(currentMenu));
        // Execute menu action here
    }
    
    void displayMenu() {
        Serial.println("Current menu: " + String(currentMenu));
    }
};

MenuSystem menuSystem;

void setup() {
    Serial.begin(9600);
    menuSystem.init();
}

void loop() {
    menuSystem.update();
    delay(10);
}
```

### State Machine with Controls

```cpp
#include "controls/controls.h"

enum SystemState {
    STATE_IDLE,
    STATE_MENU,
    STATE_SETTING,
    STATE_CONFIRM
};

class StateMachine {
private:
    SystemState currentState = STATE_IDLE;
    Controls::ButtonControl modeButton;
    Controls::RotaryEncoder valueControl;
    
public:
    StateMachine(uint8_t modePin, uint8_t rotA, uint8_t rotB, uint8_t rotBtn) 
        : modeButton(modePin), valueControl(rotA, rotB, rotBtn) {}
    
    void init() {
        modeButton.Begin();
        valueControl.Begin();
        
        modeButton.eventManager.addListener(EventManager::kEventKeyPress,
            [](int event, int param) { stateMachine.handleModeButton(); });
        valueControl.eventManager.addListener(EventManager::kEventKeyPress,
            [](int event, int param) { stateMachine.handleValueButton(); });
        valueControl.eventManager.addListener(EventManager::kEventMenu0,
            [](int event, int param) { stateMachine.handleValueChange(); });
    }
    
    void update() {
        modeButton.eventManager.processAllEvents();
        valueControl.eventManager.processAllEvents();
        modeButton.Loop();
        valueControl.Loop();
    }
    
    void handleModeButton() {
        switch(currentState) {
            case STATE_IDLE:
                currentState = STATE_MENU;
                Serial.println("Entering menu mode");
                break;
            case STATE_MENU:
                currentState = STATE_IDLE;
                Serial.println("Exiting to idle");
                break;
            case STATE_SETTING:
                currentState = STATE_CONFIRM;
                Serial.println("Confirm changes?");
                break;
            case STATE_CONFIRM:
                currentState = STATE_MENU;
                Serial.println("Changes confirmed");
                break;
        }
    }
    
    void handleValueButton() {
        if(currentState == STATE_MENU) {
            currentState = STATE_SETTING;
            Serial.println("Editing value...");
        }
    }
    
    void handleValueChange() {
        if(currentState == STATE_SETTING) {
            int value = valueControl.RotaryValue();
            Serial.println("New value: " + String(value));
        }
    }
};

StateMachine stateMachine(8, 2, 3, 4);  // Mode button on 8, rotary on 2,3,4

void setup() {
    Serial.begin(9600);
    stateMachine.init();
}

void loop() {
    stateMachine.update();
    delay(10);
}
```

## Hardware Considerations

### Button Wiring
- Connect one side of button to digital pin
- Connect other side to GND
- Internal pullup resistors are automatically enabled
- No external resistors needed

### Rotary Encoder Wiring
- Connect A and B signals to digital pins
- Connect button (if present) to digital pin
- Connect common/ground to Arduino GND
- Internal pullups automatically enabled

### Debounce Timing
- Default 50ms works for most mechanical switches
- Increase to 100-200ms for very noisy switches
- Decrease to 20-30ms for responsive tactile switches
- Very fast switches may need custom debounce logic

## Best Practices

### 1. Initialization Order
```cpp
void setup() {
    // 1. Initialize serial/other systems first
    Serial.begin(9600);
    
    // 2. Initialize controls
    button.Begin();
    
    // 3. Register event handlers
    button.eventManager.addListener(EventManager::kEventKeyPress, handler);
    
    // 4. Other setup code
}
```

### 2. Loop Structure
```cpp
void loop() {
    // 1. Process all events first
    button.eventManager.processAllEvents();
    rotary.eventManager.processAllEvents();
    
    // 2. Update control states
    button.Loop();
    rotary.Loop();
    
    // 3. Other processing
    updateDisplay();
    
    // 4. Small delay for stability
    delay(10);
}
```

### 3. Error Handling
```cpp
void setup() {
    button.Begin();
    
    if(button.ErrorCode() != 0) {
        Serial.println("Button initialization failed: " + String(button.ErrorCode()));
        // Handle error - maybe use LED indicator or reset
    }
}
```

### 4. Event Handler Best Practices
```cpp
// Good: Quick, non-blocking handlers
void handleButton(int event, int param) {
    buttonPressed = true;  // Set flag
    lastButtonTime = millis();
}

// Bad: Blocking operations in handlers
void handleButton(int event, int param) {
    delay(1000);  // Don't do this!
    Serial.println("Long operation");  // Keep serial output short
}
```

## Troubleshooting

### Common Issues

1. **Events not firing**
   - Check that `processAllEvents()` is called before `Loop()`
   - Verify event handlers are registered correctly
   - Ensure `Begin()` was called

2. **Multiple events for single press**
   - Increase debounce delay in constructor
   - Check wiring for noise/poor connections
   - Verify pullup resistors are working

3. **Rotary encoder missing steps**
   - Ensure both A and B pins are connected
   - Check for proper quadrature signals with oscilloscope
   - May need hardware filtering for very noisy environments

4. **Memory issues with many controls**
   - Each control uses EventManager with internal buffers
   - Consider increasing EventManager queue size
   - Use object pooling for dynamic controls

### Debug Techniques

```cpp
void debugControl(Controls::ButtonControl& control) {
    Serial.println("Pin: " + String(control.pin));
    Serial.println("State: " + String(control.ButtonState()));
    Serial.println("Error: " + String(control.ErrorCode()));
}
```

## Performance Notes

- Each control processes events in O(n) time where n is number of listeners
- Controls use minimal CPU when no state changes occur
- Memory usage: ~50-100 bytes per control depending on event queue size
- Suitable for real-time applications with proper loop timing

## Integration with Other Libraries

The Controls library works well with:
- LiquidCrystal displays for menu systems
- EEPROM for storing rotary encoder positions
- Servo/Motor libraries for control applications
- Communication libraries (WiFi, Bluetooth) for remote control

See the main project examples for complete integration patterns.
