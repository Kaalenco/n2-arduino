# Virtual Screen System Documentation

## Overview

The Virtual Screen System extends the 2x16 LCD display to support 16 virtual screens (numbered 0-15). Users can navigate between screens using the rotary encoder, while all screen content is stored in memory for instant switching.

## Features

- **16 Virtual Screens**: Each screen stores 2 rows × 16 characters
- **Rotary Navigation**: Turn the rotary encoder to switch between screens
- **Memory Buffering**: All screen content stored in RAM for instant display
- **Dynamic Updates**: Write to any screen, visible or not
- **Screen Persistence**: Content remains when switching between screens

## System Architecture

### Data Structure
```cpp
String virtualScreens[MAX_SCREENS][SCREEN_ROWS];
// MAX_SCREENS = 16 (screens 0-15)
// SCREEN_ROWS = 2 (rows 0-1)
```

### Navigation
- **Rotary Encoder**: Controls which screen is visible
- **Screen Selection**: `currentScreen = abs(rotaryValue) % 16`
- **Automatic Switching**: Display updates immediately when rotary moves

## API Reference

### Core Functions

#### `displayTextToScreen(int screen, int row, String text)`
Write text to a specific virtual screen.
- **screen**: Target screen number (0-15)
- **row**: Row number (0-1)
- **text**: Text content (auto-padded/truncated to 16 chars)

#### `displayText(int row, String text)`
Write text to the currently visible screen.
- **row**: Row number (0-1)
- **text**: Text content

#### `switchToScreen(int screen)`
Manually switch to a specific screen.
- **screen**: Target screen number (0-15)

#### `clearScreen(int screen)`
Clear all content from a specific screen.

#### `clearAllScreens()`
Clear content from all virtual screens.

### Utility Functions

#### `refreshCurrentScreen()`
Force refresh of the LCD with current screen content.

## Screen Layout Examples

### Default Screen Assignment
```
Screen 0: Sensor Data (Temperature, Altitude, Time)
Screen 1: Menu 1
Screen 2: Menu 2  
Screen 3: Settings
Screen 4: Clock Settings (populated in command mode)
Screen 5: Display Config (populated in command mode)
Screen 6: Sensor Config (populated in command mode)
Screens 7-15: Available for custom use
```

### Example Usage

#### Basic Display
```cpp
// Write to current screen
displayText(0, "Temperature: 22C");
displayText(1, "Altitude: 1250ft");

// Write to specific screen
displayTextToScreen(5, 0, "Brightness: 75% ");
displayTextToScreen(5, 1, "Contrast: Auto  ");
```

#### Screen Navigation
```cpp
// Switch to settings screen
switchToScreen(3);

// Navigate with rotary encoder (automatic)
// Turning encoder updates currentScreen value
```

## Integration with Controls

### Rotary Encoder
- **Rotation**: Changes `currentScreen` (0-15 based on encoder value)
- **Button Press**: Toggles command mode, populates additional screens

### Select Button
- **Normal Mode**: Toggles LCD backlight
- **Command Mode**: Performs screen-specific actions

## Command Mode Behavior

### Normal Mode (commandMode = false)
- **Select Button**: Toggles backlight
- **Screen Content**: Static information display
- **Navigation**: Free scrolling through screens

### Command Mode (commandMode = true)
- **Select Button**: Screen-specific actions
- **Screen Content**: Interactive menus and settings
- **Additional Screens**: Auto-populated with configuration options

## Memory Usage

- **Storage**: `16 screens × 2 rows × 16 chars = 512 characters`
- **String Objects**: ~1KB RAM (including String overhead)
- **Performance**: Instant screen switching, no rendering delay

## Implementation Details

### Text Formatting
- **Auto-padding**: Short text padded with spaces to 16 characters
- **Truncation**: Long text truncated to 16 characters
- **Consistent Width**: All rows exactly 16 characters for clean display

### Screen Switching Logic
```cpp
void handleRotaryEvent(int event, int param) {
    int newScreen = abs(currentRotaryValue) % MAX_SCREENS;
    if(newScreen != currentScreen) {
        switchToScreen(newScreen);
    }
}
```

### Update Behavior
- **Visible Screen**: Updates immediately on LCD
- **Hidden Screens**: Updates stored in memory only
- **Screen Switch**: Full refresh from memory buffer

## Best Practices

### Screen Organization
1. **Screen 0**: Always sensor data (default view)
2. **Screens 1-3**: Main menu items
3. **Screens 4-6**: Settings and configuration
4. **Screens 7-15**: Application-specific content

### Content Management
```cpp
// Good: Meaningful, padded content
displayTextToScreen(1, 0, "Menu Item 1     ");
displayTextToScreen(1, 1, "Select to config");

// Avoid: Content that changes too rapidly
// (can cause flicker on visible screen)
```

### Performance Tips
- Write to hidden screens when possible
- Use `refreshCurrentScreen()` sparingly
- Batch updates to reduce LCD operations

## Error Handling

### Parameter Validation
- Screen numbers outside 0-15 are ignored
- Row numbers outside 0-1 are ignored
- Text automatically formatted to prevent overflow

### Robustness
- System continues operation with invalid parameters
- Memory allocation is static (no dynamic allocation issues)
- Display updates are atomic

## Future Extensions

### Possible Enhancements
- **Screen Icons**: Visual indicators for screen type
- **Animation**: Smooth transitions between screens
- **Screen Groups**: Organize screens into categories
- **Save/Load**: Persist screen content to EEPROM
- **Templates**: Pre-defined screen layouts

### Integration Opportunities
- **Menu System**: Hierarchical navigation
- **Settings Manager**: Configuration screens
- **Data Logging**: Historical data screens
- **Notifications**: Alert/status screens

## Troubleshooting

### Common Issues
1. **Screen not updating**: Check if writing to correct screen number
2. **Content missing**: Verify row/column parameters
3. **Garbled display**: Ensure text length is appropriate
4. **Navigation issues**: Check rotary encoder connections

### Debug Functions
```cpp
// Print current screen info
Serial.println("Screen: " + String(currentScreen));
Serial.println("Content: " + virtualScreens[currentScreen][0]);
Serial.println("Content: " + virtualScreens[currentScreen][1]);
```

This virtual screen system provides a powerful foundation for complex user interfaces while maintaining the simplicity of a basic LCD display.
