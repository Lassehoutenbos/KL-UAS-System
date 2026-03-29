#ifndef VEML7700_H
#define VEML7700_H

#include <stdint.h>
#include "protocol.h"   /* als_packet_t */

/**
 * Latest ALS reading — written by veml7700_task(), readable from any task.
 * Individual field reads are effectively atomic on Cortex-M0+.
 */
extern volatile als_packet_t g_latest_als;

/**
 * Initialize the VEML7700 over I2C0.
 * I2C0 must already be initialised by digital_io_init() before calling this.
 * Configures gain=1/8, integration time=100 ms (range up to ~120 klux).
 * Must be called before starting the FreeRTOS scheduler.
 */
void veml7700_init(void);

/**
 * FreeRTOS task: reads VEML7700 ALS and WHITE registers every 500 ms,
 * converts to millilux, and enqueues a PROTO_TYPE_ALS (0x0B) packet on
 * g_tx_queue.
 */
void veml7700_task(void *param);

#endif /* VEML7700_H */
