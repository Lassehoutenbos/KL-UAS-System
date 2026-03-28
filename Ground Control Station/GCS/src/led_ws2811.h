#ifndef LED_WS2811_H
#define LED_WS2811_H

#include <stdint.h>

#define WS2811_NUM_PIXELS   2   /* LED5 and LED6 */

/**
 * Initialize PIO0 SM1 for the WS2811 chain on PIN_WS2811_DATA (GP1).
 * Must be called before starting the FreeRTOS scheduler.
 */
void led_ws2811_init(void);

/**
 * Set colour and animation mode for one RGB button (called from protocol handler).
 * button_id: 0=LED5, 1=LED6
 * r,g,b: base colour (0-255 each)
 * anim:  LED_ANIM_OFF/ON/BLINK_SLOW/BLINK_FAST/PULSE
 * Thread-safe: protected by internal volatile struct; call xTaskNotifyGive
 * on ws2811_task_handle for immediate effect (optional — task self-ticks at 50 ms).
 */
void led_ws2811_set_button(uint8_t button_id,
                           uint8_t r, uint8_t g, uint8_t b, uint8_t anim);

/**
 * Set global brightness scale for WS2811 chain (0=off, 255=full).
 * Applied on every animation tick. Thread-safe (volatile write).
 */
void led_ws2811_set_brightness(uint8_t level);

/**
 * FreeRTOS task: wakes every 50 ms (or on notification) and updates the
 * WS2811 animation state, then DMA-transfers the pixel buffer to PIO0 SM1.
 */
void ws2811_task(void *param);

#endif /* LED_WS2811_H */
