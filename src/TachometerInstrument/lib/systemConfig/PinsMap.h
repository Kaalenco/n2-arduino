#ifndef TACH_PINS_MAP_H
#define TACH_PINS_MAP_H

// RPM pulse input — must be an external interrupt pin on ATmega328
// INT0 = D2, INT1 = D3
#define PIN_RPM_PULSE           2

// Trigger edge for the magneto signal.
// FALLING = active-low pulse (open-collector / pull-up to VCC, common for magneto P-leads)
// Change to RISING if your hardware produces an active-high pulse.
#define RPM_PULSE_TRIGGER       FALLING

// MCP2515 CAN bus chip select is fixed to D10 inside CanBusMCP2515.h (SLAVESELECT = 10)
// D11: SPI MOSI  D12: SPI MISO  D13: SPI SCK  (hardware-fixed)

// I2C  (hardware-fixed on ATmega328)
// A4: SDA,  A5: SCL

// SSD1306 OLED 128×64 — common default address is 0x3C.
// Some modules use 0x3D (set by the SA0 pad on the board).
#define OLED_I2C_ADDR           0x3C
#define OLED_WIDTH              128
#define OLED_HEIGHT             64
#define OLED_RESET              -1  // no dedicated reset pin wired

#endif // TACH_PINS_MAP_H
