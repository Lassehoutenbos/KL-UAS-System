#include "board.h"

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"

static uint8_t s_duty;

void board_init(void)
{
    gpio_set_function(BOARD_PIN_PWM, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(BOARD_PIN_PWM);
    pwm_set_wrap(slice, 255);
    pwm_set_chan_level(slice, pwm_gpio_to_channel(BOARD_PIN_PWM), 0);
    pwm_set_enabled(slice, true);

    adc_init();
    adc_gpio_init(BOARD_PIN_NTC_ADC);
    adc_select_input(0);
}

void board_set_pwm(uint8_t duty)
{
    s_duty = duty;
    pwm_set_chan_level(pwm_gpio_to_slice_num(BOARD_PIN_PWM),
                       pwm_gpio_to_channel(BOARD_PIN_PWM), duty);
}

uint8_t board_get_pwm(void) { return s_duty; }

int8_t board_read_temp_c(void)
{
    uint16_t raw = adc_read();          /* 0..4095 */
    int32_t  c   = 25 + ((2048 - (int32_t)raw) * 60) / 2048;
    if (c < -40) c = -40;
    if (c > 125) c = 125;
    return (int8_t)c;
}

bool    board_overtemp(void)    { return board_read_temp_c() >= BOARD_OVERTEMP_C; }
uint8_t board_fault_flags(void) { return board_overtemp() ? FAULT_OVERTEMP : 0; }
