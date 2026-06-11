// Mapping of EEPROM addresses
// Max 512 bytes of EEPROM memory
// Hex value 0x200
// Start address: 0x100

#ifndef EEPROM_MAP_H
#define EEPROM_MAP_H

#define EEPROM_START_ADDRESS 0x0100
#define EEPROM_END_ADDRESS 0x0200

// Define the EEPROM addresses for the gauge zero offset (word = 2 bytes)
#define EEPROM_GAUGE_ZERO_OFFSET 0x0100
#define EEPROM_GAUGE_ZERO_OFFSET_END 0x0101

// Define the EEPROM addresses for the gauge speed (word = 2 byte)
#define EEPROM_GAUGE_SPEED 0x0102
#define EEPROM_GAUGE_SPEED_END 0x0103

#endif // EEPROM_MAP_H
