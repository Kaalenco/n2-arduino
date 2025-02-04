#include <Arduino.h>
#include <Wire.h>
#include "barometer.h"

using namespace std;

namespace Barometer {
    
    // Constructor
    BPM85 bpm85;

    // Read 1 byte from the BMP085 at 'address'
    char bmp085Read(unsigned char address)
    {
        Wire.beginTransmission(BMP085_ADDRESS);
        Wire.write(address);
        Wire.endTransmission();
        
        Wire.requestFrom(BMP085_ADDRESS, 1);            
        return Wire.read();
    }

    // Read 2 bytes from the BMP085
    // First byte will be from 'address'
    // Second byte will be from 'address'+1
    int bmp085ReadInt(unsigned char address)
    {
        unsigned char msb, lsb;
        
        Wire.beginTransmission(BMP085_ADDRESS);
        Wire.write(address);
        Wire.endTransmission();
        
        Wire.requestFrom(BMP085_ADDRESS, 2);
        
        msb = Wire.read();
        lsb = Wire.read();
        
        return (int) msb<<8 | lsb;
    }

    void BPM85::bmp085Calibration() {
        ac1 = bmp085ReadInt(0xAA);
        ac2 = bmp085ReadInt(0xAC);
        ac3 = bmp085ReadInt(0xAE);
        ac4 = bmp085ReadInt(0xB0);
        ac5 = bmp085ReadInt(0xB2);
        ac6 = bmp085ReadInt(0xB4);
        b1 = bmp085ReadInt(0xB6);
        b2 = bmp085ReadInt(0xB8);
        mb = bmp085ReadInt(0xBA);
        mc = bmp085ReadInt(0xBC);
        md = bmp085ReadInt(0xBE);
    }

    // Read the uncompensated temperature value
    unsigned int BPM85::bmp085ReadUT()
    {
        unsigned int ut;
        
        // Write 0x2E into Register 0xF4
        // This requests a temperature reading
        Wire.beginTransmission(BMP085_ADDRESS);
        Wire.write(0xF4);
        Wire.write(0x2E);
        Wire.endTransmission();
        
        // Wait at least 4.5ms
        delay(5);
        
        // Read two bytes from registers 0xF6 and 0xF7
        ut = bmp085ReadInt(0xF6);
        return ut;
    }

    // Read the uncompensated pressure value
    unsigned long BPM85::bmp085ReadUP()
    {
        unsigned char msb, lsb, xlsb;
        unsigned long up = 0;
        
        // Write 0x34+(OSS<<6) into register 0xF4
        // Request a pressure reading w/ oversampling setting
        Wire.beginTransmission(BMP085_ADDRESS);
        Wire.write(0xF4);
        Wire.write(0x34 + (OSS<<6));
        Wire.endTransmission();
        
        // Wait for conversion, delay time dependent on OSS
        delay(2 + (3<<OSS));
        
        // Read register 0xF6 (MSB), 0xF7 (LSB), and 0xF8 (XLSB)
        Wire.beginTransmission(BMP085_ADDRESS);
        Wire.write(0xF6);
        Wire.endTransmission();
        Wire.requestFrom(BMP085_ADDRESS, 3);
        
        msb = Wire.read();
        lsb = Wire.read();
        xlsb = Wire.read();
        
        up = (((unsigned long) msb << 16) | ((unsigned long) lsb << 8) | (unsigned long) xlsb) >> (8-OSS);
        
        return up;
    }

    bool BPM85::begin() {
        Wire.begin();
        bmp085Calibration();
        return bmp085ReadUT() != 0;
    }

    void BPM85::readPressure()
    {
        long x1, x2, x3, b3, b6, p;
        unsigned long b4, b7;
        
        long up = bmp085ReadUP();

        b6 = b5 - 4000;
        // Calculate B3
        x1 = (b2 * (b6 * b6)>>12)>>11;
        x2 = (ac2 * b6)>>11;
        x3 = x1 + x2;
        b3 = (((((long)ac1)*4 + x3)<<OSS) + 2)>>2;
        
        // Calculate B4
        x1 = (ac3 * b6)>>13;
        x2 = (b1 * ((b6 * b6)>>12))>>16;
        x3 = ((x1 + x2) + 2)>>2;
        b4 = (ac4 * (unsigned long)(x3 + 32768))>>15;
        
        b7 = ((unsigned long)(up - b3) * (50000>>OSS));
        if (b7 < 0x80000000)
            p = (b7<<1)/b4;
        else
            p = (b7/b4)<<1;
            
        x1 = (p>>8) * (p>>8);
        x1 = (x1 * 3038)>>16;
        x2 = (-7357 * p)>>16;
        p += (x1 + x2 + 3791)>>4;
        
        pressure = p;
    }

    void BPM85::readTemperature()
    {
        long x1, x2;
        int ut = bmp085ReadUT();
  
        x1 = (((long)ut - (long)ac6)*(long)ac5) >> 15;
        x2 = ((long)mc << 11)/(x1 + md);
        b5 = x1 + x2;

        tempature = ((b5 + 8)>>4);  
    }

    // Calculate altitude given a pressure reading
    // Updates temperature and pressure
    void BPM85::readAltitude()
    {
        const double p0 = 101300;     
        // Read temperature and pressure
        readTemperature();
        readPressure();

        altitude = (float)44330 * (1 - pow(((double) pressure/p0), 0.190295));
    }

    void BPM85::Calibrate(float  p)
    {
        p0 = p;
        readAltitude();
    }

    short BPM85::currentTemperature()
    {
        return tempature / 10;
    }

    double BPM85::currentPressure()
    {
        return pressure / 100;
    }

    float BPM85::currentAltitude()
    {
        return altitude;
    }
};