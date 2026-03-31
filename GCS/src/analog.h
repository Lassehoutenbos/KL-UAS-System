#ifndef ANALOG_H
#define ANALOG_H

#include "protocol.h"   /* adc_packet_t */

/**
 * Latest ADC sample — written by adc_task(), read by screen_display task.
 * Individual uint16_t reads are effectively atomic on Cortex-M0+.
 */
extern volatile adc_packet_t g_latest_adc;

/**
 * Initialize SPI0 GPIO. Must be called before starting the FreeRTOS scheduler.
 */
void analog_init(void);

/**
 * FreeRTOS task: reads MCP3208 CH0-CH5 at 100 Hz, serializes type-0x01
 * packets and enqueues them on g_tx_queue.
 */
void adc_task(void *param);

#endif /* ANALOG_H */
