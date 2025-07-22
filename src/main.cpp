#include <Arduino.h>
#include <string.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EventManager.h>
#include <barometer.h>
#include <clock.h>
#include <RTCLib.h>
#include <func.h>
#include <controls.h>

// Temperature
// https://microcontrollerslab.com/max6675-thermocouple-arduino-tutorial/

void setup();
void loop();
void handleEvents();
void raiseEvents();

// define display functions
void displayInit();
void displayText(int row, String text);
void displayTextToScreen(int screen, int row, const char* text);
void displayTextToScreen(int screen, int row, const __FlashStringHelper* text);
void switchToScreen(int screen);
void refreshCurrentScreen();
void clearScreen(int screen);
void clearAllScreens();

// define keypad functions
void keypadInit();
void keypadEvents();

// Event handler function declarations
void handleButtonReleaseEvent(int event, int param);
void handleRotaryEvent(int event, int param);
void handleClockTimeEvent(int event, int param);
void readBarometer(int event, int param);

// Button-specific handler declarations
void handleSelectButtonRelease(int event, int param);
void handleRotaryButtonRelease(int event, int param);

// Event system initialization
void initializeEventHandlers();

LiquidCrystal_I2C lcd(0x27, 16, 2);
Barometer::BPM85 barometer;
Clock::RtcClock clock;

Controls::ButtonControl selectButton(PIND4);

// A, B, button
Controls::RotaryEncoder rotary(PIND7, PIND6, PIND5);

// ========================================
// VIRTUAL SCREEN SYSTEM
// ========================================
#define MAX_SCREENS 5  
#define SCREEN_ROWS 2
#define SCREEN_COLS 16

#define SCR_SENSORS 0
#define SCR_CLOCK 1
#define SCR_SETTINGS 2
#define SCR_ENGINE 3
#define SCR_DEBUG 4

// Virtual screen storage - using char arrays instead of String objects
char virtualScreens[MAX_SCREENS][SCREEN_ROWS][SCREEN_COLS + 1];  // +1 for null terminator
int currentScreen = 0;  // Currently displayed screen (0-7)
bool screenNeedsUpdate = false;  // Flag to indicate screen refresh needed

// local memory
float altitude = 0;
bool lcdBacklightEnabled = true; // Command mode is used to enter/exit command mode
bool commandMode = false; // Example flag to indicate command mode
int currentRotaryValue = 0; // Current value of the rotary encoder

// ========================================
// SETUP FUNCTION
// ========================================
void setup() {
  Serial.begin(9600);

  Serial.println(F("N2 Arduino Setup"));

  Wire.begin();
  clock.Begin();

  selectButton.Begin();
  rotary.Begin();

  // Initialize event handlers
  initializeEventHandlers();

  // Initialize led display (0 to 7)
  // ledDisplay.setBrightness(0x03);
  // ledDisplay.clear();
  // ledDisplay.showNumberDecEx(0, 0b01000000, true);

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  displayInit();
  keypadInit();

  if(!barometer.begin()){
    Serial.println(F("BMP085 not found, check connections!"));
  }
  else
  {
    Serial.println(F("BMP085 found"));
  }

    if(!clock.RtcFound){
    Serial.println(F("RTC not found, check connections/battery"));
  }
  else
  {
    Serial.println(F("RTC found"));
    
    // Example usage of new date/time configuration functions:
    // clock.SetDateTimeComponent(Clock::YEAR_2DIGIT, 24);  // Set year to 2024
    // clock.SetDateTimeComponent(Clock::MONTH, 12);        // Set month to December
    // clock.SetDateTimeComponent(Clock::DAY, 25);          // Set day to 25th
    // clock.SetDateTimeComponent(Clock::HOUR, 14);         // Set hour to 2 PM
    // clock.SetDateTimeComponent(Clock::MINUTE, 30);       // Set minute to 30
    // clock.SetDateTimeComponent(Clock::SECOND, 0);        // Set second to 0
    // Serial.println("Current DateTime: " + clock.GetFormattedDateTime());
  }
}

void ToggleCommandMode()
{
  if(commandMode) {
    commandMode = false;
  } else {
    commandMode = true;
  }
}

void ToggleBacklight()
{
  if(lcdBacklightEnabled) {
    lcd.noBacklight();
    lcdBacklightEnabled = false;
  } else {
    lcd.backlight();
    lcdBacklightEnabled = true;
  }
}

// the loop function runs over and over again forever
void loop() {
  unsigned long currentMillis = millis();
  handleEvents();

  // do some other stuff here, like checking and validating sensor data

  raiseEvents();

  // wait for the next loop in low power mode
  unsigned long nextMillis = millis();
  unsigned long sleepTime = 200 - (nextMillis - currentMillis);
  if(sleepTime > 0)
  {
    delayMicroseconds((unsigned int)sleepTime);
  }
  
}

void handleEvents()
{
  clock.eventManager.processAllEvents();
  selectButton.eventManager.processAllEvents();
  rotary.eventManager.processAllEvents();
}

void raiseEvents()
{
  clock.Loop();
  selectButton.Loop();
  rotary.Loop();
}

// ========================================
// EVENT SYSTEM INITIALIZATION
// ========================================

void initializeEventHandlers()
{
  // The event manager uses a stack with a fixed size for event listeners and event queues.
  // Adjust the sizes in EventManager.h if needed, but keep them reasonable to avoid memory issues

  // Register barometer event handler
  if(!clock.eventManager.addListener( EventManager::EventType::kEventTimer0, readBarometer ))
  {
    Serial.println(F("Failed to add readBarometer"));
  }
 
  // Register clock time event handler
  if(!clock.eventManager.addListener( EventManager::EventType::kEventTimer0, handleClockTimeEvent ))
  {
      Serial.println(F("Failed to add handleTimeEvent"));
  }  

  // Register push button event handler
  if(!selectButton.eventManager.addListener( EventManager::EventType::kEventKeyRelease, handleButtonReleaseEvent ))
  {
      Serial.println(F("Failed to add handleButtonReleaseEvent"));
  }  

  // Register rotary encoder button event handler
  if(!rotary.eventManager.addListener( EventManager::EventType::kEventKeyRelease, handleButtonReleaseEvent ))
  {
      Serial.println(F("Failed to add handleButtonReleaseEvent"));
  }  

  // Register rotary encoder rotation event handler
  if(!rotary.eventManager.addListener( EventManager::EventType::kEventMenu0, handleRotaryEvent ))
  {
      Serial.println(F("Failed to add handleRotaryEvent"));
  }  
}

// ========================================
// EVENT HANDLERS
// ========================================

// Barometer Event Handler - reads and displays atmospheric data
void readBarometer(int event, int param){
  if(! barometer.active()) return;
  barometer.readAltitude();

  // Use Serial.print to avoid String concatenation
  Serial.print("Pressure: ");
  Serial.print(barometer.currentPressure());
  Serial.println("Hpa");

  float t = barometer.currentTemperature();
  float alt = barometer.currentAltitude();
  float altft = alt * 3.28084;
  
  // Display barometer data on screen 0 (default sensor screen) using sprintf
  char sensorBuffer[17];
  sprintf(sensorBuffer, "%3dC %4dft     ", (int)t, (int)altft);
  displayTextToScreen(SCR_SENSORS, 0, sensorBuffer);
}

// Clock Time Event Handler - displays current time on LCD
void handleClockTimeEvent(int event, int param)
{
  if(!clock.RtcFound) return;
  clock.ReadTime();

// ledDisplay.clear();
//  if(s % 2 == 0)
//    ledDisplay.showNumberDecEx(hm, 0b00000000, true);
//  else
//    ledDisplay.showNumberDecEx(hm, 0b01000000, true);

  // Display time on screen 0 (default sensor screen)
  char timeBuffer[17];
  sprintf(timeBuffer, "%02d:%02d:%02d       ", clock.hour, clock.minute, clock.second);

  displayTextToScreen(SCR_SENSORS, 1, timeBuffer);
  displayTextToScreen(SCR_CLOCK, 1, timeBuffer);
}

// ========================================
// BUTTON EVENT HANDLERS
// ========================================

// Main button event dispatcher
void handleButtonReleaseEvent(int event, int param)
{
  Serial.print("BtnRelease:");
  Serial.println(param);
  
  if(param == PIND6) {
    handleSelectButtonRelease(event, param);
  } else if(param == PIND5) {
    handleRotaryButtonRelease(event, param);
  } else {
    Serial.print("UnknownBtn:");
    Serial.println(param);
  }
}

// Select button (PIND6) handler - toggles LCD backlight

void handleSelectButtonRelease(int event, int param)
{
  // Based on the current screen and command mode, perform different actions
  if(commandMode) {
    // In command mode, select button performs screen-specific actions
    switch(currentScreen) {
      case SCR_SENSORS:
        displayTextToScreen(SCR_SENSORS, 1, F("Sensor Reset    "));
        break;
      case SCR_CLOCK:
        displayTextToScreen(SCR_CLOCK, 1, F("Menu 1 Selected "));
        break;
      case SCR_SETTINGS:
        displayTextToScreen(SCR_SETTINGS, 1, F("Settings Mode   "));
        break;
      case SCR_DEBUG:
        displayTextToScreen(SCR_DEBUG, 1, F("Debug mode      "));
        break;
      default:
        displayTextToScreen(currentScreen, 1, F("Screen Action   "));
        break;
    }
  } else {
    // Normal mode: toggle backlight
    ToggleBacklight();
    
    // Update status on all screens to show backlight state
    for(int i = 1; i < MAX_SCREENS; i++) {
      if(i != currentScreen) {  // Don't overwrite current screen content
        if(lcdBacklightEnabled) {
          displayTextToScreen(i, 1, F("Light: ON       "));
        } else {
          displayTextToScreen(i, 1, F("Light: OFF      "));
        }
      }
    }
  }
}

// Rotary button (PIND5) handler - can be used for menu selection/confirmation
void handleRotaryButtonRelease(int event, int param)
{
  ToggleCommandMode();

  // Display command mode status on current screen
  if(commandMode) {
    displayTextToScreen(currentScreen, 1, F("CMD Mode: ON    "));
  } else {
    displayTextToScreen(currentScreen, 1, F("CMD Mode: OFF   "));
  }
  
  // Also populate some example content on different screens
  if(commandMode) {
    displayTextToScreen(SCR_SETTINGS, 0, F("Clock Settings  "));
    displayTextToScreen(SCR_SETTINGS, 1, F("Set Date/Time   "));

    displayTextToScreen(SCR_DEBUG, 0, F("Display Config  "));
    displayTextToScreen(SCR_DEBUG, 1, F("Brightness/etc  "));
  }

  Serial.print("Rotary button action - current rotary value: ");
  Serial.println(rotary.RotaryValue());
  Serial.print("Current screen: ");
  Serial.print(currentScreen);
  Serial.print(", Command mode: ");
  Serial.println(commandMode);
}

// Rotary Encoder Rotation Handler - handles rotary encoder turns
void handleRotaryEvent(int event, int param)
{
  currentRotaryValue = param;
  
  // Calculate screen number (0-15) based on rotary value
  int newScreen = abs(currentRotaryValue) % MAX_SCREENS;
  
  if(newScreen != currentScreen) {
    switchToScreen(newScreen);
  }
}

// ========================================
// KEYPAD FUNCTIONS
// ========================================
int leftButtonValue = 0;
int rightButtonValue = 0;
int setButtonValue = 0;

void keypadInit()
{
  pinMode(DD3, INPUT_PULLUP);
  pinMode(DD4, INPUT_PULLUP);
  pinMode(DD2, INPUT_PULLUP);
  leftButtonValue = 0;
  rightButtonValue = 0;
  setButtonValue = 0;
}

void keypadEvents(){
  int leftButton = digitalRead(DD3);
  int rightButton = digitalRead(DD4);
  int setButton = digitalRead(DD2);
  if(leftButtonValue != leftButton)
  {
//    eventManager.queueEvent(EventManager::kEventUser0, leftButton);
    leftButtonValue = leftButton;
  }
  if(rightButtonValue != rightButton)
  {
//    eventManager.queueEvent(EventManager::kEventUser1, rightButton);
    rightButtonValue = rightButton;
  }
  if(setButtonValue != setButton)
  {
//    eventManager.queueEvent(EventManager::kEventUser2, setButton);
    setButtonValue = setButton;
  }
}

// ========================================
// VIRTUAL SCREEN DISPLAY FUNCTIONS
// ========================================

void displayInit()
{
  lcd.init();
  lcd.clear();
  lcd.backlight();
  lcdBacklightEnabled = true;
  
  // Initialize all virtual screens to empty
  clearAllScreens();
  
  // Set up default screens with labels using PROGMEM strings
  displayTextToScreen(SCR_SENSORS, 0, F("Sensors         "));
  displayTextToScreen(SCR_CLOCK, 0, F("Clock           "));
  displayTextToScreen(SCR_CLOCK, 1, F("                "));
  displayTextToScreen(SCR_SETTINGS, 0, F("Settings        "));
  displayTextToScreen(SCR_SETTINGS, 1, F("                "));
  displayTextToScreen(SCR_DEBUG, 0, F("Debug           "));
  displayTextToScreen(SCR_DEBUG, 1, F("                "));

  // Start on screen 0
  switchToScreen(0);
}

/**
 * @brief Write text to the currently visible screen
 * @param row Row number (0-1)
 * @param text Text to display (max 16 characters)
 */
void displayText(int row, char const* text)
{
  displayTextToScreen(currentScreen, row, text);
}

/**
 * @brief Write text to a specific virtual screen (PROGMEM version)
 * @param screen Screen number (0-3)
 * @param row Row number (0-1)
 * @param text Text to display from PROGMEM (max 16 characters)
 */
void displayTextToScreen(int screen, int row, const __FlashStringHelper* text)
{
  // Validate parameters
  if(screen < 0 || screen >= MAX_SCREENS) return;
  if(row < 0 || row >= SCREEN_ROWS) return;
  
  // Copy from PROGMEM to RAM buffer
  strcpy_P(virtualScreens[screen][row], (PGM_P)text);
  
  // Pad with spaces if needed
  int len = strlen(virtualScreens[screen][row]);
  for(int i = len; i < SCREEN_COLS; i++) {
    virtualScreens[screen][row][i] = ' ';
  }
  virtualScreens[screen][row][SCREEN_COLS] = '\0';  // Null terminate
  
  // If this is the currently visible screen, update the LCD immediately
  if(screen == currentScreen) {
    lcd.setCursor(0, row);
    lcd.print(virtualScreens[screen][row]);
  }
}

/**
 * @brief Write text to a specific virtual screen
 * @param screen Screen number (0-3)
 * @param row Row number (0-1)
 * @param text Text to display (max 16 characters)
 */
void displayTextToScreen(int screen, int row, char const* text)
{
  // Validate parameters
  if(screen < 0 || screen >= MAX_SCREENS) return;
  if(row < 0 || row >= SCREEN_ROWS) return;
  
  // Copy text and pad with spaces if needed
  strcpy(virtualScreens[screen][row], text);
  virtualScreens[screen][row][SCREEN_COLS] = '\0';  // Ensure null termination
  
  // Pad with spaces if needed
  int len = strlen(virtualScreens[screen][row]);
  for(int i = len; i < SCREEN_COLS; i++) {
    virtualScreens[screen][row][i] = ' ';
  }
  virtualScreens[screen][row][SCREEN_COLS] = '\0';  // Null terminate
  
  // If this is the currently visible screen, update the LCD immediately
  if(screen == currentScreen) {
    lcd.setCursor(0, row);
    lcd.print(virtualScreens[screen][row]);
  }
  
  // Simplified Serial output without String concatenation
  Serial.print("Screen ");
  Serial.print(screen);
  Serial.print(" Row ");
  Serial.print(row);
  Serial.print(": ");
  Serial.println(virtualScreens[screen][row]);
}

/**
 * @brief Switch to a different virtual screen
 * @param screen Screen number to switch to (0-15)
 */
void switchToScreen(int screen)
{
  if(screen < 0 || screen >= MAX_SCREENS) return;
  
  currentScreen = screen;
  refreshCurrentScreen();

  Serial.print(F("Screen: "));
  Serial.println(currentScreen);
}

/**
 * @brief Refresh the LCD with the current virtual screen content
 */
void refreshCurrentScreen()
{
  lcd.clear();
  
  // Display both rows of the current screen
  for(int row = 0; row < SCREEN_ROWS; row++) {
    lcd.setCursor(0, row);
    lcd.print(virtualScreens[currentScreen][row]);
  }
  
  screenNeedsUpdate = false;
}

/**
 * @brief Clear a specific virtual screen
 * @param screen Screen number to clear (0-7)
 */
void clearScreen(int screen)
{
  if(screen < 0 || screen >= MAX_SCREENS) return;
  
  for(int row = 0; row < SCREEN_ROWS; row++) {
    strcpy(virtualScreens[screen][row], "                "); // 16 spaces
  }
  
  // If this is the currently visible screen, refresh the display
  if(screen == currentScreen) {
    refreshCurrentScreen();
  }
}

/**
 * @brief Clear all virtual screens
 */
void clearAllScreens()
{
  for(int screen = 0; screen < MAX_SCREENS; screen++) {
    for(int row = 0; row < SCREEN_ROWS; row++) {
      strcpy(virtualScreens[screen][row], "                "); // 16 spaces
    }
  }
  
  // Refresh current display
  if(currentScreen >= 0 && currentScreen < MAX_SCREENS) {
    refreshCurrentScreen();
  }
}

// Legacy function for compatibility with existing setBacklight calls
int displayBacklight = 0;
void setBacklight(int event, int param)
{
  if (displayBacklight != param){
    if(param==1)
    {
      lcd.backlight();
    }
    else
    {
      lcd.noBacklight();
    }
    displayBacklight = param;
  }
}

 