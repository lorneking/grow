#ifndef CRC_H
#define CRC_H

#include <stdint.h>

#define CRC8_POLYNOMIAL 0x31
#define CRC8_INIT 0xFF

uint8_t calculate_crc(const uint8_t *data, uint16_t count);

#endif // CRC_H