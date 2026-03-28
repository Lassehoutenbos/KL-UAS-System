#ifndef LED_SK6812_H
#define LED_SK6812_H

#include <stdint.h>

#define SK6812_MAX_PIXELS   128

/**
 * Initialize PIO0 SM0 for the SK6812 chain on PIN_SK6812_DATA (GP0).
 * Must be called before starting the FreeRTOS scheduler.
 */
void led_sk6812_init(void);

/**
 * Copy pixel data into the SK6812 pixel buffer.
 * pixel_data: array of num_pixels * 3 bytes in GRB order.
 * Applies current brightness scale. Thread-safe: call from any task,
 * then xTaskNotifyGive(sk6812_task_handle) to trigger DMA transfer.
 */
void led_sk6812_set(const uint8_t *pixel_data, uint8_t num_pixels);

/**
 * Set global brightness scale for the SK6812 chain (0=off, 255=full).
 * Applied on the next call to led_sk6812_set(). Thread-safe (volatile write).
 */
void led_sk6812_set_brightness(uint8_t level);

/**
 * FreeRTOS task: blocks on task notification, then DMA-transfers
 * the pixel buffer to PIO0 SM0 TX FIFO.
 */
void sk6812_task(void *param);

#endif /* LED_SK6812_H */
