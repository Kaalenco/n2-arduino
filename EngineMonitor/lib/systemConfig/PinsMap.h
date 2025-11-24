// EMS (Engine Monitor System) Pin Mapping
// This file defines the hardware pin assignments for the engine monitoring system
// Target: Arduino Nano ATmega328
//
// Pin usage summary:
// Digital: D2-D7 (controls), D10-D13 (SPI sensors)
// Analog: A0-A3 (analog sensors)
// I2C: SDA/SCL (display, RTC, barometer)
// SPI: MISO/MOSI/SCK (CAN bus, temperature sensors)

#ifndef EMS_PINS_MAP_H
#define EMS_PINS_MAP_H

// ========================================
// SPI TEMPERATURE SENSORS
// ========================================
// MAX31855 thermocouple-to-digital converters
// All share common SPI bus (MISO/MOSI/SCK), individual chip select pins
// Hardware SPI pins on Arduino Nano:
//   MISO: D12 (fixed)
//   MOSI: D11 (fixed)
//   SCK:  D13 (fixed)

// CS for spi are used with an AND gate on PIN_SPI_CS to allow multiple devices on the same SPI bus
#define PIN_TEMP_EGT         2      // CS for exhaust gas temperature
#define PIN_TEMP_CHT1        3      // CS for cylinder head temperature 1 (optional)
#define PIN_TEMP_ENGINE      7       // CS for engine ambient temperature
#define PIN_TEMP_OIL         4      // CS for cylinder head temperature 1
#define PIN_TEMP_CHT2       A6      // CS for cylinder head temperature 2 (as digital output)
#define PIN_TEMP_CHT3       A7      // CS for cylinder head temperature 3 (as digital output)
// Note: Arduino Nano only has A0-A7. For CHT4, use D2 or D3 if needed

#define thermoCLK               5      // Clock pin for MAX31855 (shared)
#define thermoDO                6      // Data Out pin for MAX31855 (shared)

#define RESERVED                8      // Free digital pin for future use
#define PIN_BUZZER              9      // Buzzer pin for audio alerts

// Note: For systems with multiple CHT sensors (4-6 cylinders),
// additional CS pins would be needed. Consider using:
// - D8, D9 for CHT2, CHT3
// - Analog pins as digital I/O (A4-A7) for CHT4-CHT6
// But only if not using those analog pins for sensor inputs

// ========================================
// ANALOG INPUT SENSORS
// ========================================
// 10-bit ADC inputs (0-1023 = 0-5V)
// Typically used with voltage dividers or sensor modules

#define PIN_ANALOG_OIL_PRESSURE     A1  // Oil pressure sensor (0-5V output)
#define PIN_ANALOG_BATTERY_VOLTAGE  A2  // Battery voltage (via voltage divider)
#define PIN_ANALOG_BATTERY_CURRENT  A3  // Battery current sensor (hall effect/shunt)

// Note: A4 and A5 are reserved for I2C (SDA/SCL)
// A6 and A7 (Nano only) can be used as additional analog inputs if needed

// ========================================
// I2C DEVICES
// ========================================
// I2C bus uses fixed pins on Arduino Nano:
//   SDA: A4 (fixed)
//   SCL: A5 (fixed)

// I2C Device Addresses
#define I2C_ADDR_LCD            0x27    // 16x2 LCD with I2C backpack
#define I2C_ADDR_BAROMETER      0x77    // BMP085/BMP180 barometric pressure sensor
#define I2C_ADDR_RTC            0x68    // PCF8523 Real-Time Clock (default address)

// Optional I2C devices (for future expansion)
// #define I2C_ADDR_COMPASS     0x1E    // HMC5883L magnetometer (magnetic heading)
// #define I2C_ADDR_ACCELEROMETER 0x53  // ADXL345 accelerometer (for future use)


// ========================================
// HARDWARE SPI PIN DEFINITIONS
// ========================================
// For reference - these are fixed in hardware on Arduino Nano
// Do not reassign these pins for other purposes

#define PIN_SPI_CS              10      // Chip Select (fixed)
// PIN_SPI_MISO  12  (defined in SPI, Master In Slave Out (fixed))
// PIN_SPI_MOSI  11  (defined in SPI, Master Out Slave In (fixed))
// PIN_SPI_SCK   13  (defined in SPI, Serial Clock (fixed))

// ========================================
// PIN VALIDATION MACROS
// ========================================
// Helper macros to validate pin assignments at compile time

// Check if a pin is in valid digital range (2-13 on Nano)
#define IS_VALID_DIGITAL_PIN(pin) ((pin) >= 2 && (pin) <= 13)

// Check if a pin is in valid analog range (A0-A7 on Nano, A0-A3 common)
#define IS_VALID_ANALOG_PIN(pin) ((pin) >= A0 && (pin) <= A7)

// ========================================
// PIN USAGE NOTES
// ========================================
/*
 * CURRENT PIN ALLOCATION:
 *
 * Digital Pins:
 *   D0, D1:  Reserved for Serial (USB) - DO NOT USE
 *   D2:     MAX6675 CS - EGT temp
 *   D3:     MAX6675 CS - CHT1 temp (optional)
 *   D4:      MAX6675 SO (shared data out)
 *   D5:      MAX6675 CS - Ambient temp
 *   D6:      MAX6675 SCK (shared clock)
 *   D7:      MAX6675 CS - CHT3 temp
 *   D8:      CAN bus CS
 *   D9:      Buzzer for audio alerts
 *   D10:     SPI CS 
 *   D11:     SPI MOSI (shared)
 *   D12:     SPI MISO (shared)
 *   D13:     SPI SCK (shared)
 *
 * Analog Pins:
 *   A0:      Available for expansion (fuel level, etc.)
 *   A1:      Oil pressure sensor
 *   A2:      Battery voltage sensor
 *   A3:      Battery current sensor
 *   A4:      I2C SDA (reserved)
 *   A5:      I2C SCL (reserved)
 *   A6:      MAX6675 CS - CHT2 temp (can be used as digital output)
 *   A7:      MAX6675 CS - CHT3 temp (can be used as digital output)
 *
 * EXPANSION OPTIONS:
 * - Additional CHT sensor: Use D2 or D3 for CHT4 if needed
 * - Additional analog sensors: Use A0, or A6/A7 if not used for MAX6675 CS
 * - External interrupt capability: D2 (INT0), D3 (INT1) for high-priority inputs
 */

#endif // EMS_PINS_MAP_H
