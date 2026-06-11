#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <CanbusReceiver.h>

LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display
CanbusReceiver::CanbusReceiver canbusReceiver;

void setup()
{
  lcd.init();                      // initialize the lcd 
  lcd.backlight();
  Serial.begin(9600);

  canbusReceiver.begin(CanBusInterface::SPEED_500KBPS, CanBusInterface::MODE_NORMAL);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("CAN Bus Receiver");
}

void loop()
{
  // when messages are available from CAN bus
  if (canbusReceiver.isReady() && canbusReceiver.messageAvailable()) {
    Common::SensorReading readings[10];
    uint8_t count = 0;
    uint16_t msgType = canbusReceiver.receiveMessage(readings, 10, count);

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Msg Type: 0x");
    lcd.print(msgType, HEX);

    for (uint8_t i = 0; i < count; i++) {
      lcd.setCursor(0,1);
      lcd.print("ID:");
      lcd.print(readings[i].id);
      lcd.print(" Val:");
      lcd.print(readings[i].value);
      delay(500);
    }
  }
  
}