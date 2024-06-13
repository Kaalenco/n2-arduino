#pragma once
#define BMP085_ADDRESS 0x77

using namespace std;

namespace Barometer {

  class BPM85 {
    private:
    // Oversampling setting
    // set to 0, 1, 2, or 3, higher numbers are slower and 
    // use more power, typically (1 + OSS) * 3uA,
    // noise levels:
    // 0 = 0.5m, 
    // 1 = 0.4m, 
    // 2  = 0.3m
    // 3  = 0.25m
    const unsigned char OSS = 0;

    float  p0 = 101300; // Pressure at sea level (Pa)

    // Calibration values
    int ac1;
    int ac2; 
    int ac3; 
    unsigned int ac4;
    unsigned int ac5;
    unsigned int ac6;
    int b1; 
    int b2;
    int mb;
    int mc;
    int md;

    // b5 is calculated in bmp085GetTemperature(...), 
    // this variable is also used in bmp085GetPressure(...)
    // so ...Temperature(...) must be called before ...Pressure(...).
    long b5; 

    short tempature;
    long pressure;
    float altitude;

    void bmp085Calibration();
    unsigned int bmp085ReadUT();
    unsigned long bmp085ReadUP();

    public:
    bool begin();

    void readPressure();
    void readTemperature();
    void readAltitude();
    void Calibrate(float  p);

    short currentTemperature();
    double currentPressure();
    float currentAltitude();
  };
};