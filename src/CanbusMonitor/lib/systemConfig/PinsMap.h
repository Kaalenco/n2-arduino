#ifndef CANMONITOR_PINS_MAP_H
#define CANMONITOR_PINS_MAP_H

// Rotary encoder breakout (3-button module: S1, S2, KEY)
// Not a quadrature encoder — each pin is a simple active-low button.
#define PIN_ENC_S1          5   // CW step (active low, internal pull-up)
#define PIN_ENC_S2          6   // CCW step (active low, internal pull-up)
#define PIN_ENC_KEY         7   // Push button (active low, internal pull-up)

// SD card chip select (Seeed CAN Bus Shield V2.0)
#define PIN_SD_CS           4

// MCP2515 CAN chip select is fixed to D10 inside CanBusMCP2515.h (SLAVESELECT = 10)
// D11: SPI MOSI  D12: SPI MISO  D13: SPI SCK  (hardware-fixed)

// I2C (hardware-fixed on ATmega328): A4 = SDA, A5 = SCL
// DS1307 RTC I2C address (fixed by hardware)
#define DS1307_I2C_ADDR     0x68

// 16×2 character LCD with I2C backpack
// Common addresses: 0x27 (PCF8574T) or 0x3F (PCF8574AT)
#define LCD_I2C_ADDR        0x27
#define LCD_COLS            16
#define LCD_ROWS            2

// Analog inputs used when BUILD_SIMULATOR + SIMULATOR_USE_ANALOG are defined
#define PIN_SIM_ANALOG_0    A0
#define PIN_SIM_ANALOG_1    A1
#define PIN_SIM_ANALOG_2    A2
#define PIN_SIM_ANALOG_3    A3

#endif // CANMONITOR_PINS_MAP_H
