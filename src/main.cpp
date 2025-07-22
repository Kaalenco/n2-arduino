#include <Arduino.h>
#include <string.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <TM1637Display.h>
#include <EventManager.h>
#include "barometer/barometer.h"
#include "clock/clock.h"
#include "RTCLib.h"
#include "func.h"
#include "controls/controls.h"

// Temperature
// https://microcontrollerslab.com/max6675-thermocouple-arduino-tutorial/

void setup();
void loop();
void handleEvents();
void raiseEvents();

// define display functions
void displayInit();
void displayText(int row, String text);
void displayTextToScreen(int screen, int row, String text);
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

#define DM_CLK 7
#define DM_DIO 6

#define DALT_CLK 7
#define DALT_DIO 6

// TM1637Display ledDisplay(DM_CLK, DM_DIO);

// ========================================
// VIRTUAL SCREEN SYSTEM
// ========================================
#define MAX_SCREENS 16
#define SCREEN_ROWS 2
#define SCREEN_COLS 16

// Virtual screen storage - stores text for all 16 screens
String virtualScreens[MAX_SCREENS][SCREEN_ROWS];
int currentScreen = 0;  // Currently displayed screen (0-15)
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
    Serial.println(PSTR("BMP085 not found, check connections!"));
  }
  else
  {
    Serial.println(PSTR("BMP085 found"));
  }

    if(!clock.RtcFound){
    Serial.println(PSTR("RTC not found, check connections/battery"));
  }
  else
  {
    Serial.println(PSTR("RTC found"));
    
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
  // Register barometer event handler
  if(!clock.eventManager.addListener( EventManager::EventType::kEventTimer0, readBarometer ))
  {
    Serial.println(PSTR("Failed to add readBarometer listener"));
  }
 
  // Register clock time event handler
  if(!clock.eventManager.addListener( EventManager::EventType::kEventTimer0, handleClockTimeEvent ))
  {
      Serial.println(PSTR("Failed to add handleTimeEvent listener"));
  }  

  // Register push button event handler
  if(!selectButton.eventManager.addListener( EventManager::EventType::kEventKeyRelease, handleButtonReleaseEvent ))
  {
      Serial.println(PSTR("Failed to add handleButtonReleaseEvent listener"));
  }  

  // Register rotary encoder button event handler
  if(!rotary.eventManager.addListener( EventManager::EventType::kEventKeyRelease, handleButtonReleaseEvent ))
  {
      Serial.println(PSTR("Failed to add handleButtonReleaseEvent listener"));
  }  

  // Register rotary encoder rotation event handler
  if(!rotary.eventManager.addListener( EventManager::EventType::kEventMenu0, handleRotaryEvent ))
  {
      Serial.println(PSTR("Failed to add handleRotaryEvent listener"));
  }  
}

// ========================================
// EVENT HANDLERS
// ========================================

// Barometer Event Handler - reads and displays atmospheric data
void readBarometer(int event, int param){
  if(! barometer.active()) return;
  barometer.readAltitude();

  Serial.println("Pressure: " + String(barometer.currentPressure()) + "Hpa");

  float t = barometer.currentTemperature();
  float alt = barometer.currentAltitude();
  float altft = alt * 3.28084;
  
  // Display barometer data on screen 0 (default sensor screen)
  displayTextToScreen(0, 0, 
    "" + left_pad(String(t,0),3,' ') + "C" +
    " " + left_pad(String(altft,0),4,' ') + "ft");
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
  displayTextToScreen(0, 1, 
    "" 
    + left_pad(String(clock.hour), 2, '0') 
    + ":" + left_pad(String(clock.minute), 2, '0') 
    + ":" + left_pad(String(clock.second), 2, '0') 
    );
}

// ========================================
// BUTTON EVENT HANDLERS
// ========================================

// Main button event dispatcher
void handleButtonReleaseEvent(int event, int param)
{
  Serial.println(PSTR("BtnRelease:") + String(param));
  
  if(param == PIND6) {
    handleSelectButtonRelease(event, param);
  } else if(param == PIND5) {
    handleRotaryButtonRelease(event, param);
  } else {
    Serial.println(PSTR("UnknownBtn:") + String(param));
  }
}

// Select button (PIND6) handler - toggles LCD backlight

void handleSelectButtonRelease(int event, int param)
{
  // Based on the current screen and command mode, perform different actions
  if(commandMode) {
    // In command mode, select button performs screen-specific actions
    switch(currentScreen) {
      case 0:
        displayTextToScreen(0, 1, "Sensor Reset    ");
        break;
      case 1:
        displayTextToScreen(1, 1, "Menu 1 Selected ");
        break;
      case 2:
        displayTextToScreen(2, 1, "Menu 2 Selected ");
        break;
      case 3:
        displayTextToScreen(3, 1, "Settings Mode   ");
        break;
      default:
        displayTextToScreen(currentScreen, 1, "Screen " + String(currentScreen) + " Action");
        break;
    }
  } else {
    // Normal mode: toggle backlight
    ToggleBacklight();
    
    // Update status on all screens to show backlight state
    String backlightStatus = lcdBacklightEnabled ? "Light: ON       " : "Light: OFF      ";
    for(int i = 1; i < MAX_SCREENS; i++) {
      if(i != currentScreen) {  // Don't overwrite current screen content
        displayTextToScreen(i, 1, backlightStatus);
      }
    }
  }
}

// Rotary button (PIND5) handler - can be used for menu selection/confirmation
void handleRotaryButtonRelease(int event, int param)
{
  ToggleCommandMode();

  // Display command mode status on current screen
  String modeStatus = commandMode ? "CMD Mode: ON    " : "CMD Mode: OFF   ";
  
  // Show mode status on the bottom row temporarily
  displayTextToScreen(currentScreen, 1, modeStatus);
  
  // Also populate some example content on different screens
  if(commandMode) {
    displayTextToScreen(4, 0, "Clock Settings  ");
    displayTextToScreen(4, 1, "Set Date/Time   ");
    
    displayTextToScreen(5, 0, "Display Config  ");
    displayTextToScreen(5, 1, "Brightness/etc  ");
    
    displayTextToScreen(6, 0, "Sensor Config   ");
    displayTextToScreen(6, 1, "Calibration     ");
  }

  Serial.println(PSTR("Rotary button action - current rotary value: ") + String(rotary.RotaryValue()));
  Serial.println(PSTR("Current screen: ") + String(currentScreen) + PSTR(", Command mode: ") + String(commandMode));
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
  
  // Set up default screens with labels
  displayTextToScreen(0, 0, "Sensors         ");
  displayTextToScreen(1, 0, "Menu 1          ");
  displayTextToScreen(1, 1, "                ");
  displayTextToScreen(2, 0, "Menu 2          ");
  displayTextToScreen(2, 1, "                ");
  displayTextToScreen(3, 0, "Settings        ");
  displayTextToScreen(3, 1, "                ");
  
  // Start on screen 0
  switchToScreen(0);
}

/**
 * @brief Write text to the currently visible screen
 * @param row Row number (0-1)
 * @param text Text to display (max 16 characters)
 */
void displayText(int row, String text)
{
  displayTextToScreen(currentScreen, row, text);
}

/**
 * @brief Write text to a specific virtual screen
 * @param screen Screen number (0-15)
 * @param row Row number (0-1)
 * @param text Text to display (max 16 characters)
 */
void displayTextToScreen(int screen, int row, String text)
{
  // Validate parameters
  if(screen < 0 || screen >= MAX_SCREENS) return;
  if(row < 0 || row >= SCREEN_ROWS) return;
  
  // Pad or truncate text to exactly 16 characters
  String paddedText = text;
  while(paddedText.length() < SCREEN_COLS) {
    paddedText += " ";
  }
  if(paddedText.length() > SCREEN_COLS) {
    paddedText = paddedText.substring(0, SCREEN_COLS);
  }
  
  // Store text in virtual screen buffer
  virtualScreens[screen][row] = paddedText;
  
  // If this is the currently visible screen, update the LCD immediately
  if(screen == currentScreen) {
    lcd.setCursor(0, row);
    lcd.print(paddedText);
  }
  
  Serial.println("Screen " + String(screen) + " Row " + String(row) + ": " + paddedText);
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
  
  Serial.println(PSTR("Screen: ") + String(currentScreen));
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
 * @param screen Screen number to clear (0-15)
 */
void clearScreen(int screen)
{
  if(screen < 0 || screen >= MAX_SCREENS) return;
  
  for(int row = 0; row < SCREEN_ROWS; row++) {
    virtualScreens[screen][row] = "                "; // 16 spaces
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
      virtualScreens[screen][row] = "                "; // 16 spaces
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

 