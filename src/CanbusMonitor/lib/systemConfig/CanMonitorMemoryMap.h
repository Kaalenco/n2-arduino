#ifndef CANMONITOR_MEMORY_MAP_H
#define CANMONITOR_MEMORY_MAP_H

// EEPROM layout — 0x0100 to 0x010F (16 bytes reserved)

// Magic byte: confirms EEPROM was initialised with valid defaults.
// If absent on boot, defaults are written automatically.
#define EEPROM_CM_MAGIC         0x0100  // uint8_t
#define EEPROM_CM_MAGIC_VALUE   0xCB

// CAN bus speed index (uint8_t):
//   0 = 125 kbps, 1 = 250 kbps, 2 = 500 kbps
// Change via serial command SPEED:<kbps>; applied on next reset.
#define EEPROM_CM_CAN_SPEED     0x0101  // uint8_t

// Default written on first boot
#define CM_DEFAULT_CAN_SPEED    2       // 500 kbps

#endif // CANMONITOR_MEMORY_MAP_H
