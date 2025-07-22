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

// define keypad functions
void keypadInit();
void keypadEvents();

// Event handler function declarations
void handlePushButtonReleaseEvent(int event, int param);
void handleRotaryEvent(int event, int param);
void handleClockTimeEvent(int event, int param);
void readBarometer(int event, int param);

// Event system initialization
void initializeEventHandlers();

LiquidCrystal_I2C lcd(0x27, 16, 2);
Barometer::BPM85 barometer;
Clock::RtcClock clock;

Controls::ButtonControl pushButton(PIND4);

// A, B, button
Controls::RotaryEncoder rotary(PIND7, PIND6, PIND5);

#define DM_CLK 7
#define DM_DIO 6

#define DALT_CLK 7
#define DALT_DIO 6

// TM1637Display ledDisplay(DM_CLK, DM_DIO);

float altitude = 0;

void setup() {
  Serial.begin(9600);

  Wire.begin();
  clock.Begin();
  pushButton.Begin();
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
    Serial.println("Sensor BMP085 not found, check the connections!");
  }
  else
  {
    Serial.println("Sensor BMP085 found");
  }

    if(!clock.RtcFound){
    Serial.println("Real time clock not found, check the connections and battery");
  }
  else
  {
    Serial.println("Real time clock found");
    
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
  pushButton.eventManager.processAllEvents();
  rotary.eventManager.processAllEvents();
}

void raiseEvents()
{
  clock.Loop();
  pushButton.Loop();
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
    Serial.println("Failed to add readBarometer listener");
  }
 
  // Register clock time event handler
  if(!clock.eventManager.addListener( EventManager::EventType::kEventTimer0, handleClockTimeEvent ))
  {
      Serial.println("Failed to add handleTimeEvent listener");
  }  

  // Register push button event handler
  if(!pushButton.eventManager.addListener( EventManager::EventType::kEventKeyRelease, handlePushButtonReleaseEvent ))
  {
      Serial.println("Failed to add handlePushButtonReleaseEvent listener");
  }  

  // Register rotary encoder button event handler
  if(!rotary.eventManager.addListener( EventManager::EventType::kEventKeyRelease, handlePushButtonReleaseEvent ))
  {
      Serial.println("Failed to add handlePushButtonReleaseEvent listener");
  }  

  // Register rotary encoder rotation event handler
  if(!rotary.eventManager.addListener( EventManager::EventType::kEventMenu0, handleRotaryEvent ))
  {
      Serial.println("Failed to add handleRotaryEvent listener");
  }  
}

// ========================================
// EVENT HANDLERS
// ========================================

// Barometer Event Handler
void readBarometer(int event, int param){
  if(! barometer.active()) return;
  barometer.readAltitude();

  Serial.println("Pressure: " + String(barometer.currentPressure()) + "Hpa");

  float t = barometer.currentTemperature();
  float alt = barometer.currentAltitude();
  float altft = alt * 3.28084;
  displayText(0, 
    "" + left_pad(String(t,0),3,' ') + "C" +
    " " + left_pad(String(altft,0),4,' ') + "ft");
}

// Clock Time Event Handler
void handleClockTimeEvent(int event, int param)
{
  if(!clock.RtcFound) return;
  clock.ReadTime();

// ledDisplay.clear();
//  if(s % 2 == 0)
//    ledDisplay.showNumberDecEx(hm, 0b00000000, true);
//  else
//    ledDisplay.showNumberDecEx(hm, 0b01000000, true);

  displayText(1, 
    "" 
    + left_pad(String(clock.hour), 2, '0') 
    + ":" + left_pad(String(clock.minute), 2, '0') 
    + ":" + left_pad(String(clock.second), 2, '0') 
    );
}

// Push Button Event Handler
bool pushButtonToggle = false;
void handlePushButtonReleaseEvent(int event, int param)
{
  Serial.println("Push button released: " + String(param) );
  if(param == PIND6)
  {
    Serial.println("Push button on D6 released");
    if(pushButtonToggle)
    {
      lcd.noBacklight();
      pushButtonToggle = false;
    }
    else 
    {
      lcd.backlight();
      pushButtonToggle = true;
    }
  }
}

// Rotary Encoder Event Handler
void handleRotaryEvent(int event, int param)
{
  Serial.println("Rotary value: " + String(param) );
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
// DISPLAY FUNCTIONS
// ========================================
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

void displayInit()
{
  lcd.init();
  lcd.clear();
  lcd.backlight();
  displayBacklight = 0;
}

void displayText(int row, String text)
{
  lcd.setCursor(0, row);
  lcd.print(text);
}

 