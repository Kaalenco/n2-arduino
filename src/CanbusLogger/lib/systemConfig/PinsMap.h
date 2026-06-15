#ifndef CANLOGGER_PINS_MAP_H
#define CANLOGGER_PINS_MAP_H

// SD card chip select (Seeed CAN Bus Shield V2.0)
#define PIN_SD_CS           4

// MCP2515 CAN chip select is fixed to D10 inside CanBusMCP2515.h (SLAVESELECT = 10)
// D11: SPI MOSI  D12: SPI MISO  D13: SPI SCK  (hardware-fixed)

// I2C (hardware-fixed on ATmega328): A4 = SDA, A5 = SCL
// DS1307 RTC I2C address (fixed by hardware)
#define DS1307_I2C_ADDR     0x68

// Optional status LED — used to indicate logging state.
// Choose a pin that doesn't conflict with SPI (10-13) or I2C (A4-A5).
#define PIN_STATUS_LED      2

#endif // CANLOGGER_PINS_MAP_H
