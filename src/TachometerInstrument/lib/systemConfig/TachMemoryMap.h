#ifndef TACH_MEMORY_MAP_H
#define TACH_MEMORY_MAP_H

// EEPROM layout — 0x0100 to 0x010F (16 bytes reserved)

// Magic byte: confirms EEPROM was initialised with valid defaults.
// If not present on boot, defaults are written automatically.
#define EEPROM_TACH_MAGIC           0x0100  // uint8_t
#define EEPROM_TACH_MAGIC_VALUE     0xAC

// Pulses per revolution from the magneto or ignition pickup.
// Typical: 1 for single-fire magneto, 2 for dual-fire or distributor-based sources.
#define EEPROM_TACH_PULSES_PER_REV  0x0101  // uint8_t

// Over-speed warning threshold in RPM (little-endian uint16_t).
// CAN flag TACH_FLAG_OVER_SPEED is set and the display warns when exceeded.
#define EEPROM_TACH_MAX_RPM         0x0102  // uint16_t (2 bytes)

// CAN message ID used when transmitting RPM frames (little-endian uint16_t).
// Configure differently for magneto 1 (default 0x0C0) and magneto 2 (0x0C1)
// so both can coexist on the same bus.
#define EEPROM_TACH_CAN_ID          0x0104  // uint16_t (2 bytes)

// Defaults written on first boot
#define TACH_DEFAULT_PULSES_PER_REV  1
#define TACH_DEFAULT_MAX_RPM         2800
#define TACH_DEFAULT_CAN_ID          0x0C0

#endif // TACH_MEMORY_MAP_H
