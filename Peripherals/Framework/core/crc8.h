#ifndef RS485_CRC8_H
#define RS485_CRC8_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t crc8(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif
