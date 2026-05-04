#include "hal.h"

#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"

/* Pins are weak so a board can override them at link time, otherwise these
   defaults match the master side (GCS) wiring conventions on a Pico. */
#ifndef HAL_RP2040_UART
#define HAL_RP2040_UART  uart0
#endif
#ifndef HAL_RP2040_PIN_TX
#define HAL_RP2040_PIN_TX 0
#endif
#ifndef HAL_RP2040_PIN_RX
#define HAL_RP2040_PIN_RX 1
#endif
#ifndef HAL_RP2040_PIN_DE
#define HAL_RP2040_PIN_DE 2
#endif
#ifndef HAL_RP2040_PIN_INT
#define HAL_RP2040_PIN_INT 3
#endif

void hal_uart_init(uint32_t baud)
{
    uart_init(HAL_RP2040_UART, baud);
    gpio_set_function(HAL_RP2040_PIN_TX, GPIO_FUNC_UART);
    gpio_set_function(HAL_RP2040_PIN_RX, GPIO_FUNC_UART);

    gpio_init(HAL_RP2040_PIN_DE);
    gpio_set_dir(HAL_RP2040_PIN_DE, GPIO_OUT);
    gpio_put(HAL_RP2040_PIN_DE, 0);
}

void hal_uart_set_tx_enable(bool enable)
{
    gpio_put(HAL_RP2040_PIN_DE, enable ? 1 : 0);
}

void hal_uart_write(const uint8_t *buf, size_t len)
{
    uart_write_blocking(HAL_RP2040_UART, buf, len);
    uart_tx_wait_blocking(HAL_RP2040_UART);
}

int hal_uart_read(uint8_t *byte_out)
{
    if (!uart_is_readable(HAL_RP2040_UART)) return 0;
    *byte_out = (uint8_t)uart_getc(HAL_RP2040_UART);
    return 1;
}

void hal_int_pin_init(void)
{
    /* Open-drain emulation: leave the pin as input (high-Z) when released
       so the master's pull-up wins; switch to output-low to assert. */
    gpio_init(HAL_RP2040_PIN_INT);
    gpio_set_dir(HAL_RP2040_PIN_INT, GPIO_IN);
    gpio_put(HAL_RP2040_PIN_INT, 0);
}

void hal_int_pin_drive(bool assert_low)
{
    if (assert_low) {
        gpio_put(HAL_RP2040_PIN_INT, 0);
        gpio_set_dir(HAL_RP2040_PIN_INT, GPIO_OUT);
    } else {
        gpio_set_dir(HAL_RP2040_PIN_INT, GPIO_IN);
    }
}

uint32_t hal_millis(void)
{
    return (uint32_t)(to_ms_since_boot(get_absolute_time()));
}
