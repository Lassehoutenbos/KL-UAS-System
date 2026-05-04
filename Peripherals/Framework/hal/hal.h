#ifndef RS485_HAL_H
#define RS485_HAL_H

/* Portable HAL interface for the RS-485 slave framework. Each MCU port
   provides one .c file implementing every function in this header. The
   core (rs485_slave.c) includes only this header — no MCU-specific
   headers leak into portable code. */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initialise the UART at `baud` (8N1, no flow control) and configure the
   DE/RE pin as output (driven low = receive). Safe to call once at boot. */
void hal_uart_init(uint32_t baud);

/* Drive the RS-485 transceiver DE/RE pin. true = transmit, false = receive.
   Ports that wire DE to the UART driver itself (e.g. ESP-IDF RS-485 mode)
   may make this a no-op — the symbol must still exist. */
void hal_uart_set_tx_enable(bool enable);

/* Blocking write. Must not return until the last byte has fully clocked
   out of the UART shift register, so the caller can deassert DE without
   truncating the frame. */
void hal_uart_write(const uint8_t *buf, size_t len);

/* Non-blocking single-byte read. Returns 1 if a byte was available (stored
   in *byte_out), 0 if not. Never blocks. */
int  hal_uart_read(uint8_t *byte_out);

/* Configure the /INT pin as open-drain output, idle high (de-asserted). */
void hal_int_pin_init(void);

/* Drive /INT. true = pull low (assert), false = release (pulled high by
   the master's pull-up). */
void hal_int_pin_drive(bool assert_low);

/* Free-running millisecond counter. Wraps at 2^32 ms (~49 days) — callers
   compare with subtraction so wrap is fine. */
uint32_t hal_millis(void);

#ifdef __cplusplus
}
#endif

#endif
