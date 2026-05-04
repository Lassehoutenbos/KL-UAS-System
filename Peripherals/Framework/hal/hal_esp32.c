#include "hal.h"

#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_timer.h"

/* Defaults — boards can override on the compiler command line. */
#ifndef HAL_ESP32_UART
#define HAL_ESP32_UART       UART_NUM_1
#endif
#ifndef HAL_ESP32_PIN_TX
#define HAL_ESP32_PIN_TX     17
#endif
#ifndef HAL_ESP32_PIN_RX
#define HAL_ESP32_PIN_RX     16
#endif
#ifndef HAL_ESP32_PIN_DE
#define HAL_ESP32_PIN_DE     4              /* RTS — driver toggles this */
#endif
#ifndef HAL_ESP32_PIN_INT
#define HAL_ESP32_PIN_INT    5
#endif
#ifndef HAL_ESP32_RX_BUF
#define HAL_ESP32_RX_BUF     512
#endif

void hal_uart_init(uint32_t baud)
{
    uart_config_t cfg = {
        .baud_rate = (int)baud,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(HAL_ESP32_UART, HAL_ESP32_RX_BUF, 0, 0, NULL, 0);
    uart_param_config(HAL_ESP32_UART, &cfg);
    /* DE on RTS — driver auto-toggles in RS-485 half-duplex mode. */
    uart_set_pin(HAL_ESP32_UART,
                 HAL_ESP32_PIN_TX, HAL_ESP32_PIN_RX,
                 HAL_ESP32_PIN_DE, UART_PIN_NO_CHANGE);
    uart_set_mode(HAL_ESP32_UART, UART_MODE_RS485_HALF_DUPLEX);
}

void hal_uart_set_tx_enable(bool enable)
{
    /* No-op: the driver toggles DE around uart_write_bytes. The function
       must still exist so the core compiles unchanged. */
    (void)enable;
}

void hal_uart_write(const uint8_t *buf, size_t len)
{
    uart_write_bytes(HAL_ESP32_UART, (const char *)buf, len);
    uart_wait_tx_done(HAL_ESP32_UART, portMAX_DELAY);
}

int hal_uart_read(uint8_t *byte_out)
{
    int n = uart_read_bytes(HAL_ESP32_UART, byte_out, 1, 0);
    return n > 0 ? 1 : 0;
}

void hal_int_pin_init(void)
{
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << HAL_ESP32_PIN_INT),
        .mode         = GPIO_MODE_OUTPUT_OD,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);
    gpio_set_level(HAL_ESP32_PIN_INT, 1);     /* released */
}

void hal_int_pin_drive(bool assert_low)
{
    gpio_set_level(HAL_ESP32_PIN_INT, assert_low ? 0 : 1);
}

uint32_t hal_millis(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}
