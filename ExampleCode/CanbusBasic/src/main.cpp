#include "main.h"
#include "coreFunctions.h"

// Define the gauge type and pins
//Gauge::AutoGauge gauge(Gauge::X27_168, 5,6,7,8);
//int stepsPerRev = gauge.StepsPerRevolution(Gauge::X27_168);
//int startMillis = millis();

int altimeter = 0;
float temperatures[SENSOR_TEMPERATURE_END - SENSOR_TEMPERATURE];

void remoteRequest(int event, int param)
{
  // Remote request for data
}

void canError(int event, int param)
{
  // Data lost in the fifo stack of the canbus
  Serial.println("Data lost");
}

void dataReceived(int event, int param)
{
  // Data available in the fifo stack of the canbus
  // Read the data and process it
  while(param > 0)
  {
    unsigned char* data = canSensor.readMessage();
    int iValue=0;
    float fValue=0.0;
    uint8_t dataType = 0;
    bool validData = false;
    char strValue[50];
    memset(strValue, 0, sizeof(strValue));

    if(data != NULL)
    {
      dataType = data[1];
      // Process the data
      switch (data[0])
      {
        case Canbus::kMessageTypeInteger:          
          iValue = ToInt(data[2], data[3]);
          if(dataType == SENSOR_ALTIMETER)
          {
            altimeter = iValue;
            validData = true;
            strcat(strValue, "Altimeter ");
            strcat(strValue, String(iValue).c_str());
          }
          break;
        case Canbus::kMessageTypeFloat:          
          fValue = ToFloat(data[2], data[3], data[4], data[5]);
          if(dataType>= SENSOR_TEMPERATURE && dataType < SENSOR_TEMPERATURE_END)
          {
            temperatures[dataType - SENSOR_TEMPERATURE] = fValue;
            validData = true;
            strcat(strValue, "Temperature");
            strcat(strValue, String(dataType - SENSOR_TEMPERATURE).c_str());
            strcat(strValue, " ");
            strcat(strValue, String(fValue).c_str());
          }          
          break;
        default:
          break;
      }
    }
    else
    {
      // No data available
      Serial.println(F("No data available"));
    }
    if(validData)
    {
      // Data is valid
      Serial.print(F("Data received: "));
      Serial.print(dataType);
      Serial.print(' ');
      Serial.println(strValue);
    }
    param--;
  }  
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);

  // find the id or define it here
  // Now 0x01 is the id for this item

  // On nano, clock is 8Mhz, no startup message
  canSensor.begin(0x02, Canbus::Clock8MHZ, false);

  // Initialize the gauge with a speed of 100
  //gauge.begin();

  canSensor.eventManager.addListener(EventManager::EventType::kEventRequestData, remoteRequest);
  canSensor.eventManager.addListener(EventManager::EventType::kEventCanDataReceived, dataReceived);
  canSensor.eventManager.addListener(EventManager::EventType::kEventCanDataError, canError);
  
  // use noise to seed the random number generator
  randomSeed(analogRead(0));
}

void loop() {
  // put your main code here, to run repeatedly
  unsigned long currentMillis = millis();
  handleEvents();
  raiseEvents();

  // Output data every second and when data is requested
  if(currentMillis - lastTimerEvent > 1000)
  {
      lastTimerEvent = currentMillis;

      canSensor.sendSensorFloat(SENSOR_ALTIMETER, currentMillis / 7);

      digitalWrite(LED_BUILTIN, HIGH);   
      delay(100);
      digitalWrite(LED_BUILTIN, LOW);   
  }

  // print a random number from 10 to 19
  //int randNumber = random(0, stepsPerRev * 2 - stepsPerRev);
  //gauge.setTargetValue(randNumber);
}

void handleEvents()
{
  canSensor.eventManager.processAllEvents();
  //gauge.eventManager.processAllEvents();
}

void raiseEvents()
{
  canSensor.loop();
  //gauge.loop();
}