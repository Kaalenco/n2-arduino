// EMS (Engine Monitor System) EEPROM Memory Map
// This file defines the EEPROM memory layout for engine monitoring parameters,
// alarm thresholds, and sensor calibration data
//
// Memory organization:
// 0x0100-0x0103: Legacy gauge settings (preserved for compatibility)
// 0x0104-0x0150: EMS alarm thresholds and limits
// 0x0151-0x01A0: EMS sensor calibration data
// 0x01A1-0x01FF: Reserved for future use

#ifndef EMS_MEMORY_MAP_H
#define EMS_MEMORY_MAP_H

#include "EepromMap.h"

// ========================================
// BAROMETER / ALTITUDE SETTINGS
// ========================================
// QNH setting for altitude calculation (2 bytes, stored as int16_t representing hPa * 10)
// Example: 1013.2 hPa is stored as 10132
#define EEPROM_QNH_SETTING          0x0104
#define EEPROM_QNH_SETTING_END      0x0105

// Altitude alarm thresholds (2 bytes each, stored as int16_t in feet)
#define EEPROM_ALT_WARN_HIGH        0x0106  // High altitude warning (e.g., 10000 ft)
#define EEPROM_ALT_WARN_HIGH_END    0x0107
#define EEPROM_ALT_WARN_LOW         0x0108  // Low altitude warning (e.g., 500 ft AGL)
#define EEPROM_ALT_WARN_LOW_END     0x0109

// ========================================
// TEMPERATURE ALARM THRESHOLDS
// ========================================
// All temperatures stored as int16_t in degrees Celsius

// Ambient temperature limits (2 bytes each)
#define EEPROM_TEMP_AMB_WARN_HIGH   0x010A  // High ambient temp warning
#define EEPROM_TEMP_AMB_WARN_HIGH_END 0x010B
#define EEPROM_TEMP_AMB_WARN_LOW    0x010C  // Low ambient temp warning (icing concern)
#define EEPROM_TEMP_AMB_WARN_LOW_END 0x010D

// EGT (Exhaust Gas Temperature) limits (2 bytes each)
#define EEPROM_TEMP_EGT_CAUTION     0x010E  // EGT caution threshold (e.g., 750°C)
#define EEPROM_TEMP_EGT_CAUTION_END 0x010F
#define EEPROM_TEMP_EGT_WARNING     0x0110  // EGT warning threshold (e.g., 850°C)
#define EEPROM_TEMP_EGT_WARNING_END 0x0111

// CHT (Cylinder Head Temperature) limits (2 bytes each)
#define EEPROM_TEMP_CHT_CAUTION     0x0112  // CHT caution threshold (e.g., 200°C)
#define EEPROM_TEMP_CHT_CAUTION_END 0x0113
#define EEPROM_TEMP_CHT_WARNING     0x0114  // CHT warning threshold (e.g., 230°C)
#define EEPROM_TEMP_CHT_WARNING_END 0x0115

// Oil temperature limits (2 bytes each)
#define EEPROM_TEMP_OIL_CAUTION_HIGH 0x0116 // Oil temp high caution (e.g., 105°C)
#define EEPROM_TEMP_OIL_CAUTION_HIGH_END 0x0117
#define EEPROM_TEMP_OIL_WARNING_HIGH 0x0118 // Oil temp high warning (e.g., 115°C)
#define EEPROM_TEMP_OIL_WARNING_HIGH_END 0x0119
#define EEPROM_TEMP_OIL_WARN_LOW    0x011A  // Oil temp low warning (e.g., 40°C minimum)
#define EEPROM_TEMP_OIL_WARN_LOW_END 0x011B

// ========================================
// PRESSURE ALARM THRESHOLDS
// ========================================
// Oil pressure limits (2 bytes each, stored as int16_t in PSI * 10)
// Example: 25.5 PSI is stored as 255
#define EEPROM_PRESS_OIL_CAUTION_LOW 0x011C // Oil pressure low caution
#define EEPROM_PRESS_OIL_CAUTION_LOW_END 0x011D
#define EEPROM_PRESS_OIL_WARNING_LOW 0x011E // Oil pressure low warning
#define EEPROM_PRESS_OIL_WARNING_LOW_END 0x011F
#define EEPROM_PRESS_OIL_WARN_HIGH  0x0120  // Oil pressure high warning
#define EEPROM_PRESS_OIL_WARN_HIGH_END 0x0121

// ========================================
// FUEL SYSTEM ALARM THRESHOLDS
// ========================================
// Fuel level warnings (2 bytes each, stored as percentage * 10)
// Example: 25.5% is stored as 255
#define EEPROM_FUEL_WARN_LOW        0x0122  // Low fuel warning (e.g., 25%)
#define EEPROM_FUEL_WARN_LOW_END    0x0123
#define EEPROM_FUEL_CAUTION_LOW     0x0124  // Low fuel caution (e.g., 10%)
#define EEPROM_FUEL_CAUTION_LOW_END 0x0125

// ========================================
// ELECTRICAL SYSTEM ALARM THRESHOLDS
// ========================================
// Battery voltage limits (2 bytes each, stored as int16_t in volts * 100)
// Example: 12.5V is stored as 1250
#define EEPROM_BATT_VOLT_WARN_LOW   0x0126  // Low voltage warning (e.g., 11.5V)
#define EEPROM_BATT_VOLT_WARN_LOW_END 0x0127
#define EEPROM_BATT_VOLT_WARN_HIGH  0x0128  // High voltage warning (e.g., 15.5V)
#define EEPROM_BATT_VOLT_WARN_HIGH_END 0x0129

// Battery current limits (2 bytes each, stored as int16_t in amps * 10)
#define EEPROM_BATT_CURRENT_WARN_HIGH 0x012A // High current warning
#define EEPROM_BATT_CURRENT_WARN_HIGH_END 0x012B

// ========================================
// SENSOR CALIBRATION DATA
// ========================================
// Sensor calibration offsets (2 bytes each, stored as int16_t)

// Temperature sensor offsets (in degrees Celsius * 10)
#define EEPROM_CAL_TEMP_AMB_OFFSET  0x0151
#define EEPROM_CAL_TEMP_AMB_OFFSET_END 0x0152
#define EEPROM_CAL_TEMP_EGT_OFFSET  0x0153
#define EEPROM_CAL_TEMP_EGT_OFFSET_END 0x0154
#define EEPROM_CAL_TEMP_CHT1_OFFSET 0x0155
#define EEPROM_CAL_TEMP_CHT1_OFFSET_END 0x0156
#define EEPROM_CAL_TEMP_CHT2_OFFSET 0x0157
#define EEPROM_CAL_TEMP_CHT2_OFFSET_END 0x0158
#define EEPROM_CAL_TEMP_CHT3_OFFSET 0x0159
#define EEPROM_CAL_TEMP_CHT3_OFFSET_END 0x015A
#define EEPROM_CAL_TEMP_CHT4_OFFSET 0x015B
#define EEPROM_CAL_TEMP_CHT4_OFFSET_END 0x015C
#define EEPROM_CAL_TEMP_OIL_OFFSET  0x015D
#define EEPROM_CAL_TEMP_OIL_OFFSET_END 0x015E

// Fuel level sensor calibration (4 bytes total)
// Empty reading (2 bytes, ADC value when tank is empty)
#define EEPROM_CAL_FUEL_EMPTY       0x015F
#define EEPROM_CAL_FUEL_EMPTY_END   0x0160
// Full reading (2 bytes, ADC value when tank is full)
#define EEPROM_CAL_FUEL_FULL        0x0161
#define EEPROM_CAL_FUEL_FULL_END    0x0162

// Oil pressure sensor calibration (4 bytes total)
// Zero PSI reading (2 bytes, ADC value at 0 PSI)
#define EEPROM_CAL_OIL_PRESS_ZERO   0x0163
#define EEPROM_CAL_OIL_PRESS_ZERO_END 0x0164
// Scale factor (2 bytes, ADC counts per PSI * 10)
#define EEPROM_CAL_OIL_PRESS_SCALE  0x0165
#define EEPROM_CAL_OIL_PRESS_SCALE_END 0x0166

// Battery voltage divider ratio (2 bytes, stored as ratio * 100)
// Example: If voltage divider is 3.0:1, store as 300
#define EEPROM_CAL_BATT_VOLT_RATIO  0x0167
#define EEPROM_CAL_BATT_VOLT_RATIO_END 0x0168

// Battery current sensor calibration (4 bytes total)
#define EEPROM_CAL_BATT_CURR_ZERO   0x0169  // Zero current ADC value
#define EEPROM_CAL_BATT_CURR_ZERO_END 0x016A
#define EEPROM_CAL_BATT_CURR_SCALE  0x016B  // ADC counts per amp * 10
#define EEPROM_CAL_BATT_CURR_SCALE_END 0x016C

// Magnetic heading calibration (2 bytes, deviation offset in degrees)
#define EEPROM_CAL_MAG_HEADING_OFFSET 0x016D
#define EEPROM_CAL_MAG_HEADING_OFFSET_END 0x016E

// ========================================
// SYSTEM CONFIGURATION FLAGS
// ========================================
// Configuration byte 1 (bit flags)
#define EEPROM_CONFIG_FLAGS_1       0x0180
// Bit 0: Enable EGT alarms
// Bit 1: Enable CHT alarms
// Bit 2: Enable oil temp alarms
// Bit 3: Enable oil pressure alarms
// Bit 4: Enable fuel alarms
// Bit 5: Enable battery voltage alarms
// Bit 6: Enable altitude alarms
// Bit 7: Audio alarm enable

// Configuration byte 2 (bit flags)
#define EEPROM_CONFIG_FLAGS_2       0x0181
// Bit 0: Use Celsius (0) or Fahrenheit (1)
// Bit 1: Use meters (0) or feet (1)
// Bit 2: Use liters (0) or gallons (1)
// Bit 3-7: Reserved

// Display configuration
#define EEPROM_LCD_BRIGHTNESS       0x0182  // LCD brightness (0-255)
#define EEPROM_LCD_CONTRAST         0x0183  // LCD contrast (0-255)

// Update intervals (2 bytes each, in milliseconds)
#define EEPROM_UPDATE_INTERVAL_FAST 0x0184  // Fast sensor update interval (e.g., 250ms)
#define EEPROM_UPDATE_INTERVAL_FAST_END 0x0185
#define EEPROM_UPDATE_INTERVAL_SLOW 0x0186  // Slow sensor update interval (e.g., 1000ms)
#define EEPROM_UPDATE_INTERVAL_SLOW_END 0x0187

// ========================================
// DEFAULT VALUES
// ========================================
// These constants define factory default values for EEPROM initialization

// Default QNH (1013.25 hPa, stored as 10132)
#define DEFAULT_QNH_SETTING         10132

// Default temperature limits (Celsius)
#define DEFAULT_EGT_CAUTION         750
#define DEFAULT_EGT_WARNING         850
#define DEFAULT_CHT_CAUTION         200
#define DEFAULT_CHT_WARNING         230
#define DEFAULT_OIL_TEMP_CAUTION    105
#define DEFAULT_OIL_TEMP_WARNING    115
#define DEFAULT_OIL_TEMP_LOW        40

// Default pressure limits (PSI * 10)
#define DEFAULT_OIL_PRESS_CAUTION   250  // 25.0 PSI
#define DEFAULT_OIL_PRESS_WARNING   150  // 15.0 PSI
#define DEFAULT_OIL_PRESS_HIGH      800  // 80.0 PSI

// Default fuel limits (percentage * 10)
#define DEFAULT_FUEL_WARNING        250  // 25%
#define DEFAULT_FUEL_CAUTION        100  // 10%

// Default battery voltage limits (volts * 100)
#define DEFAULT_BATT_VOLT_LOW       1150 // 11.5V
#define DEFAULT_BATT_VOLT_HIGH      1550 // 15.5V

// Default configuration flags
#define DEFAULT_CONFIG_FLAGS_1      0xFF // All alarms enabled
#define DEFAULT_CONFIG_FLAGS_2      0x02 // Feet for altitude, Celsius for temp

// Default update intervals (milliseconds)
#define DEFAULT_UPDATE_FAST         250  // 250ms (4 Hz)
#define DEFAULT_UPDATE_SLOW         1000 // 1000ms (1 Hz)

#endif // EMS_MEMORY_MAP_H
