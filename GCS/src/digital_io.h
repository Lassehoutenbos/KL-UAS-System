#ifndef DIGITAL_IO_H
#define DIGITAL_IO_H

#include <stdbool.h>
#include <stdint.h>
#include "protocol.h"   /* digital_packet_t */

/**
 * Latest debounced digital I/O state — written by digital_io_task().
 * Read by screen_display task.
 */
extern volatile digital_packet_t g_latest_digital;

/**
 * Initialize I2C0 and the MCP23017:
 *   - Port A bits 0-2: outputs (indicator LEDs 2-4)
 *   - Port A bits 5-7: inputs with pull-ups (KEY, SW3_3, SW3_4)
 *   - Port B bits 0-7: inputs with pull-ups (SW1_1..SW3_2)
 * Must be called before starting the FreeRTOS scheduler.
 */
void digital_io_init(void);

/**
 * FreeRTOS task: reads MCP23017, applies debounce, emits input-event
 * packets on state changes, and enqueues full type-0x02 state packets.
 */
void digital_io_task(void *param);

/**
 * Set or clear one indicator LED on MCP23017 port A (call from digital_io_task only).
 * led_bit: IOEXP_A_LED2, IOEXP_A_LED3, or IOEXP_A_LED4.
 */
void digital_io_set_led(uint8_t led_bit, bool on);

/**
 * Post a single-LED set/clear request from any task.
 */
void digital_io_set_led_async(uint8_t led_bit, bool on);

/**
 * Post a mask-based LED update from any task (used by protocol handler for chain=0x02).
 * mask: bitmask of LEDs to update (bit0=LED2, bit1=LED3, bit2=LED4)
 * state: bitmask of target state (1=on, 0=off) for each masked LED
 */
void digital_io_set_leds_async(uint8_t mask, uint8_t state);

#endif /* DIGITAL_IO_H */
