#ifndef CANMONITOR_PINS_MAP_H
#define CANMONITOR_PINS_MAP_H

// Rotary encoder
#define PIN_ENCODER_CLK     2   // CLK — polling on rising/falling edge
#define PIN_ENCODER_DT      3   // DT  — sampled when CLK changes
#define PIN_ENCODER_BTN     4   // Push button (active low, internal pull-up)

// MCP2515 CAN chip select is fixed to D10 inside CanBusMCP2515.h (SLAVESELECT = 10)
// D11: SPI MOSI  D12: SPI MISO  D13: SPI SCK  (hardware-fixed)

// I2C (hardware-fixed on ATmega328): A4 = SDA, A5 = SCL

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
