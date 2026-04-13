#ifndef RS485_H
#define RS485_H

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"

/* RS-485 bus frame SOF — distinct from CDC SOF (0xAA) */
#define RS485_SOF               0xAB

/* RS-485 CMD bytes */
#define RS485_CMD_PING          0x01
#define RS485_CMD_PONG          0x02
#define RS485_CMD_SET_OUTPUT    0x10
#define RS485_CMD_GET_STATUS    0x11
#define RS485_CMD_STATUS        0x12
#define RS485_CMD_SET_PARAM     0x20
#define RS485_CMD_GET_PARAM     0x21
#define RS485_CMD_PARAM_VAL     0x22
#define RS485_CMD_STREAM_ON     0x30
#define RS485_CMD_STREAM_OFF    0x31
#define RS485_CMD_STREAM_DATA   0x32
#define RS485_CMD_ERROR         0xF0
#define RS485_CMD_SYNC          0xFF

#define RS485_ADDR_BROADCAST    0xFF
#define RS485_TIMEOUT_MS        50
#define RS485_PING_INTERVAL_MS  1000
#define RS485_MAX_PERIPHERALS   8

/**
 * Initialise UART1 and GPIO for the RS-485 bus.
 * Must be called before the FreeRTOS scheduler starts.
 */
void rs485_init(void);

/**
 * FreeRTOS task: PING sweep, command forwarding, /INT handling.
 * Priority 2 — same as cdc_task.
 */
void rs485_task(void *arg);

/**
 * Queue a command received from the Pi (CDC PROTO_TYPE_PERIPH_CMD)
 * for forwarding onto the RS-485 bus.
 * payload format: [addr, cmd, len, payload...]
 */
void rs485_forward_cmd(const uint8_t *payload, uint16_t len);

/**
 * Copy current peripheral addresses and online flags into caller buffers.
 * Returns the number of entries written (up to max_entries).
 */
uint8_t rs485_get_peripherals(uint8_t *addrs_out, bool *online_out,
                               uint8_t max_entries);

#endif /* RS485_H */
