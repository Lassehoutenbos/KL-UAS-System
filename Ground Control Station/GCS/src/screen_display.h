#ifndef SCREEN_DISPLAY_H
#define SCREEN_DISPLAY_H

/* Screen render modes (match protocol screen_cmd_t.mode) */
#define SCREEN_MODE_AUTO        0   /* state-machine driven */
#define SCREEN_MODE_MAIN        1
#define SCREEN_MODE_WARNING     2
#define SCREEN_MODE_LOCK        3
#define SCREEN_MODE_BATWARNING  4

/**
 * Initialise the ST7735 display and set initial state.
 * Must be called after g_spi1_mutex is created and SPI1 GPIO is configured.
 */
void screen_display_init(void);

/**
 * FreeRTOS task: renders screen content based on g_sys_state (or host
 * override received via xTaskNotify). Updates at 5 Hz (200 ms period).
 */
void screen_task(void *param);

#endif /* SCREEN_DISPLAY_H */
