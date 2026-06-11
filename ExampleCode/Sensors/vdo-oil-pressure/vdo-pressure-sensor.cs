/*
Original code from https://forum.arduino.cc/t/vdo-oil-pressure-sender/1147835

1200ohm - 5v - arduino - VDO oil pressure sender
Arduino is connected to the sender like this:

5v -- 1200ohm ---|----- VDO sender ----- GND
A0 --------------|

Raw values is mesuured with a ANALOG gauge. To be more precise.. USE A DIGITAL PRESSURE GAUGE
67 = 0bar
145 = 1bar
235 = 2bar
248 = 2,2bar
288 = 3bar
352 = 4bar
386 = 5bar
441 = 6bar
475 = 7bar


PLease note.. the VDO signal i NOT LINEAR, this is why i made it like this... to compensate for the unlinear signal.
Link to VDO signal curve: https://support.wellandpower.net/hc/en-us/articles/360002119317-How-do-you-test-an-oil-pressue-sender-
*/

const int analogInPin = A0;  // Analog input pin

int sensorValue = 0;        // value read from the pot
int outputValue = 0;        // value output
float output = 0;           // Final decimal output.
int stat = 101;             // Debug var.

void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(9600);
}

void loop() {
  sensorValue = analogRead(analogInPin); // Get RAW value from VDO

  switch (sensorValue) {
  case 0 ... 66: // 3 - 4bar
    stat = 0;
    output = 0;
    break;
  case 67 ... 145: // 0 - 1bar
  stat = 1;
    sensorValue = map(sensorValue, 67, 145, 10, 31);
    outputValue = map(sensorValue, 10, 31, 0, 100);
    output = (float(outputValue) / 100);
    break;
  case 146 ... 235: // 1 - 2bar
  stat = 2;
    sensorValue = map(sensorValue, 146, 235, 31, 52);
    outputValue = map(sensorValue, 31, 52, 100, 200);
    output = (float(outputValue) / 100);
    break;
  case 236 ... 248: // 2 - 2,2bar
  stat = 3;
    sensorValue = map(sensorValue, 236, 248, 52, 66);
    outputValue = map(sensorValue, 52, 66, 100, 120);
    output = (float((outputValue + 100)) / 100);
    break;
  case 249 ... 288: // 2,2 - 3bar
  stat = 4;
    sensorValue = map(sensorValue, 249, 288, 66, 71);
    outputValue = map(sensorValue, 66, 71, 220, 300);
    output = (float(outputValue) / 100);
    break;
  case 289 ... 352: // 3 - 4bar
  stat = 5;
    sensorValue = map(sensorValue, 289, 352, 71, 90);
    outputValue = map(sensorValue, 71, 90, 300, 400);
    output = (float(outputValue) / 100);
    break;
  case 353 ... 386: // 4 - 5bar
  stat = 6;
    sensorValue = map(sensorValue, 353, 386, 90, 107);
    outputValue = map(sensorValue, 90, 107, 400, 500);
    output = (float(outputValue) / 100);
    break;
  case 387 ... 441: // 5 - 6bar
  stat = 7;
    sensorValue = map(sensorValue, 387, 441, 107, 124);
    outputValue = map(sensorValue, 107, 124, 500, 600);
    output = (float(outputValue) / 100);
    break;
  case 442 ... 457: // 6 - 7bar
  stat = 8;
    sensorValue = map(sensorValue, 442, 475, 124, 140);
    outputValue = map(sensorValue, 124, 140, 600, 700);
    output = (float(outputValue) / 100);
    break;
}

  // print the results to the Serial Monitor:
  Serial.print("sensor = ");
  Serial.print(analogRead(analogInPin));
  Serial.print("\t stat = ");
  Serial.print(stat);
  Serial.print("\t lol = ");
  Serial.print(outputValue);
  Serial.print("\t output = ");
  Serial.println(output);

  delay(100);
}