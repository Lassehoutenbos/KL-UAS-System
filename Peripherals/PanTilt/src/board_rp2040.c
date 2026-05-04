#include "board.h"

#include "pico/stdlib.h"
#include "hardware/pwm.h"

static uint8_t s_angle[2];

static void servo_init_pin(uint pin)
{
    /* 50 Hz PWM, 20 ms period. Pulse width 1.0–2.0 ms = 0–180°.
       Wrap = 25000 with clkdiv = 100 → tick = 1 µs at 125 MHz. */
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(pin);
    pwm_set_clkdiv(slice, 100.0f);
    pwm_set_wrap(slice, 25000);
    pwm_set_chan_level(slice, pwm_gpio_to_channel(pin), 1500);
    pwm_set_enabled(slice, true);
}

static void servo_set(uint pin, uint8_t deg)
{
    if (deg > 180) deg = 180;
    uint16_t us = 1000 + (uint16_t)(((uint32_t)deg * 1000) / 180);
    pwm_set_chan_level(pwm_gpio_to_slice_num(pin),
                       pwm_gpio_to_channel(pin), us);
}

void board_init(void)
{
    servo_init_pin(BOARD_PIN_SERVO_PAN);
    servo_init_pin(BOARD_PIN_SERVO_TILT);
}

void board_set_angle(uint8_t ch, uint8_t deg)
{
    if (ch > 1) return;
    s_angle[ch] = deg;
    servo_set(ch == 0 ? BOARD_PIN_SERVO_PAN : BOARD_PIN_SERVO_TILT, deg);
}

uint8_t board_get_angle(uint8_t ch) { return ch < 2 ? s_angle[ch] : 0; }
uint8_t board_is_moving(void)        { return 0; }
