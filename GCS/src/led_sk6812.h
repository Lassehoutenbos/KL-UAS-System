#ifndef LED_SK6812_H
#define LED_SK6812_H

#include <stdint.h>

#define SK6812_MAX_PIXELS       128
#define WORKLIGHT_LED_BASE      60
#define WORKLIGHT_LED_COUNT     23

/**
 * Initialize PIO0 SM0 for the SK6812 chain on PIN_SK6812_DATA (GP0).
 * Must be called before starting the FreeRTOS scheduler.
 */
void led_sk6812_init(void);

/**
 * Copy pixel data into the SK6812 pixel buffer.
 * pixel_data: array of num_pixels * 4 bytes in GRBW order.
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
 * Set the warning severity for a single panel icon (LEDs 61-69).
 * icon:     0-8, use WARN_ICON_* constants from protocol.h
 * severity: WARN_OK / WARN_WARNING / WARN_CRITICAL
 * Thread-safe (volatile byte write). The sk6812_task picks up the
 * new state on its next 50 ms tick.
 */
void led_sk6812_set_warning_state(uint8_t icon, uint8_t severity);

/**
 * Set worklight state (LEDs WORKLIGHT_LED_BASE .. +WORKLIGHT_LED_COUNT-1).
 * on=0: off. on=1: solid colour r/g/b.
 * Thread-safe (volatile writes). sk6812_task picks up on next 50 ms tick.
 */
void led_sk6812_set_worklight(uint8_t on, uint8_t r, uint8_t g, uint8_t b);

/**
 * FreeRTOS task: wakes on task notification or every 50 ms, updates
 * warning panel pixels, then DMA-transfers the pixel buffer to PIO0 SM0.
 */
void sk6812_task(void *param);

#endif /* LED_SK6812_H */
