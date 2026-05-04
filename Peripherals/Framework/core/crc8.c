#include "crc8.h"

/* CRC-8/MAXIM (polynomial 0x31, init 0x00). Identical to the master in
   GCS/src/rs485.c so frames produced or checked here interoperate byte-
   for-byte with the GCS bus. */
uint8_t crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
            crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
    }
    return crc;
}
