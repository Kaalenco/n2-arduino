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

// Device ID (uint16_t little-endian): identifies this monitor on the bus.
// Used in SD log folder/file names and in the DEVID: serial command.
#define EEPROM_CM_DEVICE_ID     0x0102  // uint16_t (2 bytes)

// Aircraft identifier (uint16_t little-endian): recorded on every CSV log row.
// Set via AIRCRAFT:<hex4> serial command.
#define EEPROM_CM_AIRCRAFT_ID   0x0104  // uint16_t (2 bytes)

// Defaults written on first boot
#define CM_DEFAULT_CAN_SPEED    2       // 500 kbps
#define CM_DEFAULT_DEVICE_ID    0x0001
#define CM_DEFAULT_AIRCRAFT_ID  0x0000

#endif // CANMONITOR_MEMORY_MAP_H
