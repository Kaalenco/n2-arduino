#ifndef CANLOGGER_MEMORY_MAP_H
#define CANLOGGER_MEMORY_MAP_H

// EEPROM layout — 0x0100 to 0x010F (16 bytes reserved)

#define EEPROM_CLG_MAGIC         0x0100  // uint8_t
#define EEPROM_CLG_MAGIC_VALUE   0xCC    // distinct from CanbusMonitor (0xCB)

// CAN bus speed index (uint8_t): 0=125kbps  1=250kbps  2=500kbps
#define EEPROM_CLG_CAN_SPEED     0x0101

// Device ID (uint16_t little-endian): used in SD folder/file names.
#define EEPROM_CLG_DEVICE_ID     0x0102

// Aircraft identifier (uint16_t little-endian): written on every CSV row.
#define EEPROM_CLG_AIRCRAFT_ID   0x0104

// Defaults written on first boot
#define CLG_DEFAULT_CAN_SPEED    2       // 500 kbps
#define CLG_DEFAULT_DEVICE_ID    0x0002  // distinct from CanbusMonitor default (0x0001)
#define CLG_DEFAULT_AIRCRAFT_ID  0x0000

#endif // CANLOGGER_MEMORY_MAP_H
