#ifndef SCREEN_DISPLAY_H
#define SCREEN_DISPLAY_H

#include <stdint.h>

/* Screen render modes (match protocol screen_cmd_t.mode) */
#define SCREEN_MODE_AUTO          0   /* state-machine driven */
#define SCREEN_MODE_MAIN          1
#define SCREEN_MODE_WARNING       2
#define SCREEN_MODE_LOCK          3
#define SCREEN_MODE_BATWARNING    4
#define SCREEN_MODE_PERIPH        5   /* peripheral overview list      */
#define SCREEN_MODE_PERIPH_DETAIL 6   /* single peripheral detail view */

/**
 * Initialise the ST7735 display and set initial state.
 * Must be called before starting the FreeRTOS scheduler.
 */
void screen_display_init(void);

/**
 * FreeRTOS task: renders screen content based on g_sys_state (or host
 * override received via xTaskNotify). Updates at 5 Hz (200 ms period).
 */
void screen_task(void *param);

/**
 * Select which peripheral address the detail screen will display.
 * Resets the cached payload. Safe to call from any task context.
 */
void screen_periph_set_detail_addr(uint8_t addr);

/**
 * Push a fresh STATUS or STREAM_DATA payload into the detail screen cache.
 * Only takes effect when addr matches the currently selected detail address.
 * Safe to call from any task context.
 */
void screen_periph_update_data(uint8_t addr, uint8_t cmd,
                               const uint8_t *payload, uint8_t len);

#endif /* SCREEN_DISPLAY_H */
