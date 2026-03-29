#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/watchdog.h"

#include "FreeRTOS.h"
#include "task.h"

#include "system_state.h"
#include "protocol.h"
#include "analog.h"
#include "digital_io.h"
#include "led_sk6812.h"
#include "led_ws2811.h"
#include "usb_cdc.h"
#include "screen_display.h"
#include "veml7700.h"

/* Task handles — needed so protocol.c can notify LED/screen tasks */
static TaskHandle_t s_sk6812_handle = NULL;
static TaskHandle_t s_ws2811_handle  = NULL;
static TaskHandle_t s_screen_handle  = NULL;

/* Watchdog service task */
static void watchdog_task(void *param)
{
    (void)param;

    while (1) {
        watchdog_update();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* FreeRTOS stack overflow hook */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    proto_send_error(ERR_STACK_OVERFLOW);
    taskDISABLE_INTERRUPTS();
    while (1) {}
}

/* FreeRTOS malloc failed hook */
void vApplicationMallocFailedHook(void)
{
    proto_send_error(ERR_MALLOC_FAILED);
    taskDISABLE_INTERRUPTS();
    while (1) {}
}

int main(void)
{
    /* UART stdio only — TinyUSB owns USB hardware */
    stdio_init_all();

    /* Set initial state */
    sys_state_set(SYS_BOOT);

    /* Report watchdog reset as error (checked before re-enabling watchdog) */
    bool wdog_reset = watchdog_caused_reboot();

    /* ---- SPI1 mutex (must exist before analog_init or screen_display_init) ---- */
    g_spi1_mutex = xSemaphoreCreateMutex();
    configASSERT(g_spi1_mutex != NULL);

    /* ---- Hardware init (before scheduler) ---- */
    analog_init();
    digital_io_init();
    veml7700_init();
    led_sk6812_init();
    led_ws2811_init();
    screen_display_init();   /* GPIO-only init; full ST7735 init runs in screen_task */

    sys_state_set(SYS_INIT);

    /* ---- Shared TX queue ---- */
    g_tx_queue = xQueueCreate(TX_QUEUE_DEPTH, sizeof(tx_item_t));
    configASSERT(g_tx_queue != NULL);

    /* ---- Create tasks ---- */
    /* USB device task — must be highest priority for timely enumeration */
    xTaskCreate(usb_device_task, "USB",    1024, NULL, 4, NULL);

    /* CDC task — TX queue consumer + RX protocol handler + heartbeat */
    xTaskCreate(cdc_task,        "CDC",    512,  NULL, 3, NULL);

    /* Sensor tasks */
    xTaskCreate(adc_task,        "ADC",    512,  NULL, 2, NULL);
    xTaskCreate(digital_io_task, "DIO",    512,  NULL, 2, NULL);
    xTaskCreate(veml7700_task,   "ALS",    512,  NULL, 2, NULL);

    /* LED tasks */
    xTaskCreate(sk6812_task,     "SK6812", 512,  NULL, 1, &s_sk6812_handle);
    xTaskCreate(ws2811_task,     "WS2811", 512,  NULL, 1, &s_ws2811_handle);

    /* Screen task */
    xTaskCreate(screen_task,     "SCREEN", 1024, NULL, 1, &s_screen_handle);

    /* Watchdog service task */
    xTaskCreate(watchdog_task,   "WDT",    256,  NULL, 1, NULL);

    /* Pass task handles to protocol layer */
    proto_set_led_task_handles(s_sk6812_handle, s_ws2811_handle);
    proto_set_screen_task_handle(s_screen_handle);

    /* ---- Watchdog: enable after tasks created ---- */
    if (wdog_reset) {
        /* Will send error packet once CDC task starts */
        sys_state_set(SYS_ERROR);
    }
    watchdog_enable(5000, true);  /* 5 s timeout, pause during debug */

    /* ---- Start scheduler (does not return) ---- */
    vTaskStartScheduler();

    while (1) {}
}
