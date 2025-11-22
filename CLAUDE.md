# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an Arduino-based data collection and user interaction system for aircraft instrumentation, built using PlatformIO. The system reads multiple sensors (temperature, pressure, altitude, fuel level, etc.) and displays information on a 16x2 LCD with a virtual screen system for navigation.

## Build & Development Commands

### PlatformIO Commands
```bash
# Build the project
pio run

# Upload to board (Arduino Nano ATmega328)
pio run --target upload

# Open serial monitor (9600 baud)
pio device monitor

# Clean build artifacts
pio run --target clean

# Build and upload in one command
pio run --target upload && pio device monitor
```

### Target Hardware
- **Board**: Arduino Nano ATmega328
- **Alternative**: Use `nanoatmega328old` in platformio.ini for old bootloader
- **Monitor Speed**: 9600 baud

## Architecture Overview

### Event-Driven System
The codebase uses an event-driven architecture based on the Arduino-EventManager library. Each control and sensor module has its own `EventManager` instance.

**Critical Pattern**: In the main loop, ALWAYS call event handlers BEFORE raising new events:
```cpp
void loop() {
    handleEvents();  // Process queued events first
    // ... other processing ...
    raiseEvents();   // Then poll controls and queue new events
}
```

The `handleEvents()` function processes all event queues:
```cpp
void handleEvents() {
    clock.eventManager.processAllEvents();
    selectButton.eventManager.processAllEvents();
    rotary.eventManager.processAllEvents();
}
```

The `raiseEvents()` function polls controls and raises events:
```cpp
void raiseEvents() {
    clock.Loop();
    selectButton.Loop();
    rotary.Loop();
}
```

### Virtual Screen System
The display system uses a virtual screen architecture allowing 16 logical screens (currently using 5) on a single 16x2 LCD:

- **Screen Storage**: `char virtualScreens[MAX_SCREENS][SCREEN_ROWS][SCREEN_COLS + 1]`
- **Navigation**: Rotary encoder cycles through screens (value mod MAX_SCREENS)
- **Updates**: Can write to any screen; only current screen shows on LCD
- **Screen IDs**:
  - `SCR_SENSORS` (0): Main sensor display
  - `SCR_CLOCK` (1): Clock display
  - `SCR_SETTINGS` (2): Settings menu
  - `SCR_ENGINE` (3): Engine parameters
  - `SCR_DEBUG` (4): Debug information

**Key Functions**:
- `displayTextToScreen(screen, row, text)`: Write to specific screen
- `switchToScreen(screen)`: Change visible screen
- Use `F()` macro for string literals to save RAM: `displayTextToScreen(0, 0, F("Text"))`

### Library Structure

The project uses custom libraries in `lib/`:

**Controls Library** (`lib/controls/`):
- `ButtonControl`: Debounced button with press/release events
- `RotaryEncoder`: Quadrature encoder with integrated button
- Inherits from ButtonControl, adds rotation tracking
- Events: `kEventKeyPress`, `kEventKeyRelease` (button), `kEventMenu0` (rotation)
- Must call `Begin()` before `addListener()` to properly initialize EventManager

**Event System** (`lib/eventmanager/`):
- Fixed-size event queues (default: 8 events)
- Fixed-size listener lists (default: 8 listeners)
- Adjust `EVENTMANAGER_LISTENER_LIST_SIZE` and `EVENTMANAGER_EVENT_QUEUE_SIZE` in EventManager.h if needed
- **Warning**: Listeners must be static or global functions; use lambdas or function pointers

**Clock Library** (`lib/clock/`):
- RTC_PCF8523 interface
- Generates `kEventTimer0` events periodically
- Date/time configuration via `SetDateTimeComponent(component, value)`
- Available components: `YEAR_2DIGIT`, `MONTH`, `DAY`, `HOUR`, `MINUTE`, `SECOND`

**Barometer Library** (`lib/barometer/`):
- BMP085 sensor interface (I2C address 0x77)
- Provides pressure, temperature, and calculated altitude
- QNH calibration support via `Calibrate(float p)`
- Check `active()` before reading sensor data

**MAX6675 Library** (`lib/max6675Sensor/`):
- Thermocouple-to-digital converter interface
- Uses hardware SPI for communication
- Supports temperature offset for negative temperature ranges
- Initialize with `begin(chipSelectPin, temperatureOffset)`

**Sensor Configuration** (`lib/systemConfig/`):
- `SensorTypes.h`: Sensor type constants (temperature, EGT, CHT, etc.)
- `EepromMap.h`: EEPROM memory layout (512 bytes, start: 0x0100)

### Sensor Data Flow

Sensors are read in the clock timer event handler (`handleClockTimeEvent`):
```cpp
void handleClockTimeEvent(int event, int param) {
    readClockTime();
    readAltitude();
    readAmbientTemperature();
    readEGT();
    readOilTemperature();
    readFuelLevel();
    readBatteryVoltage();
}
```

Data is stored in `SensorData[]` array with defined indices (see SENSOR_* constants in main.cpp:91-106).

### Temperature Sensors (MAX6675)

Four MAX6675 thermocouple interfaces using hardware SPI:
- `tempAmbient` (CS pin 10): Ambient with -20°C offset for negative temps
- `tempEGT` (CS pin 11): Exhaust Gas Temperature
- `tempCHT1` (CS pin 12): Cylinder Head Temperature 1
- `tempOIL` (CS pin 13): Oil Temperature

Initialize with `begin(chipSelectPin, temperatureOffset)`.

### Pin Assignments

**Digital Pins**:
- D2-D4: Keypad (currently unused in event system)
- D4: Select button (PIND6 in button handler)
- D5: Rotary encoder button (PIND5)
- D6: Rotary encoder B (PIND6)
- D7: Rotary encoder A (PIND7)
- D10: MAX6675 ambient temperature (CS_PIN_OUTSIDE)
- D11: MAX6675 EGT (CS_PIN_EGT)
- D12: MAX6675 CHT1 (CS_PIN_CHT1)
- D13: MAX6675 oil temperature (CS_PIN_OIL)

**Analog Pins**:
- A0: Fuel level sensor
- A1: Oil pressure sensor
- A2: Battery voltage sensor

**I2C** (Wire library):
- LCD: 0x27
- BMP085 Barometer: 0x77
- RTC PCF8523: Default I2C address

## Code Patterns & Conventions

### Memory Management
- **Avoid String concatenation**: Use `Serial.print()` multiple times instead of creating String objects
- **Use PROGMEM**: Store strings in flash with `F()` macro
- **Fixed arrays**: Virtual screens use char arrays, not String objects
- **sprintf for formatting**: Use `sprintf()` for complex string formatting into char buffers

Example:
```cpp
// Good
char buffer[17];
sprintf(buffer, "%3dC %4dft", temp, altitude);
displayTextToScreen(0, 1, buffer);

// Also good (for constants)
displayTextToScreen(0, 0, F("Settings"));

// Avoid
String text = "Temp: " + String(temp) + "C";
displayTextToScreen(0, 1, text);
```

### Event Handler Registration

Register event handlers in `initializeEventHandlers()`:
```cpp
clock.eventManager.addListener(EventManager::kEventTimer0, handleClockTimeEvent);
```

**Important**: Check return value - `addListener()` returns false if listener list is full.

### Command Mode Pattern

The system has a two-mode operation:
- **Normal Mode**: Basic interaction (backlight toggle, screen navigation)
- **Command Mode**: Enabled via rotary button, unlocks screen-specific actions

Toggle with `ToggleCommandMode()`. Check `commandMode` boolean in handlers.

## Common Tasks

### Adding a New Sensor
1. Define sensor constant in SensorTypes.h (if needed)
2. Add array index to main.cpp (e.g., `#define SENSOR_NEW_THING 14`)
3. Expand `SensorData[]` array size
4. Create `readNewSensor()` function
5. Call from `handleClockTimeEvent()`
6. Update display function to show data on appropriate screen

### Adding a New Screen
1. Increment `MAX_SCREENS` constant
2. Define screen constant (e.g., `#define SCR_NEW 5`)
3. Initialize screen content in `displayInit()`
4. Add display update logic in appropriate event handler
5. Optionally add command mode actions in `handleSelectButtonRelease()`

### Modifying Event Handlers
- Event handlers have signature: `void handler(int event, int param)`
- Button events: `param` is the pin number that triggered the event
- Rotary events: `param` is the rotary counter value (for kEventMenu0)
- Timer events: `param` is typically unused or contains time information

### Working with the Controls Library
All controls follow this pattern:
```cpp
Controls::ButtonControl button(PIN);

void setup() {
    button.Begin();  // Must call BEFORE addListener
    button.eventManager.addListener(EventManager::kEventKeyPress, handler);
}

void loop() {
    button.eventManager.processAllEvents();  // Process first
    button.Loop();  // Then update state and queue new events
}
```

**Important**: The order in `setup()` matters - call `Begin()` before `addListener()` to ensure the EventManager is properly initialized.

## Debugging

- Serial output at 9600 baud provides detailed logging
- Check `RtcFound` and sensor `active()` status on initialization
- Event registration failures are logged to Serial
- Screen content is echoed to Serial when updated
- Use `Serial.print(F("text"))` for debug strings to save RAM

## Architecture Decisions

The project uses Architecture Decision Records (ADRs) in `docs/adr/`:
- **00002**: PlatformIO chosen over Visual Micro
- **00003**: Event-driven architecture for responsive UI
- See ADRs for detailed context and consequences
