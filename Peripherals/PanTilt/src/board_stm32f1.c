#include "board.h"

#include "stm32f1xx.h"

/* PanTilt on the KL-GCS-MODBUS03 board uses the J6 expansion header,
   PB10 + PB11. Both are TIM2_CH3/CH4 with TIM2 full remap (AFIO_MAPR
   TIM2_REMAP = 11). We only enable CH3 and CH4 outputs, so the CH1/CH2
   remap targets (PA15 / PB3, JTAG by default) stay untouched.

   Timer clock: APB1 = 32 MHz with /2 prescaler → TIMxCLK = 64 MHz.
   PSC = 64 - 1 → 1 MHz tick → ARR = 20000 - 1 → 20 ms (50 Hz). */

#undef BOARD_PIN_SERVO_PAN
#undef BOARD_PIN_SERVO_TILT
#define BOARD_PIN_SERVO_PAN   10u   /* PB10 = TIM2_CH3 (full remap) */
#define BOARD_PIN_SERVO_TILT  11u   /* PB11 = TIM2_CH4 (full remap) */

static uint8_t s_angle[2];

static uint16_t deg_to_us(uint8_t deg)
{
    if (deg > 180) deg = 180;
    return (uint16_t)(1000u + ((uint32_t)deg * 1000u) / 180u);
}

void board_init(void)
{
    /* Clocks: GPIOB, AFIO, TIM2. */
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    /* TIM2 full remap → CH3 on PB10, CH4 on PB11. */
    AFIO->MAPR = (AFIO->MAPR & ~AFIO_MAPR_TIM2_REMAP) | AFIO_MAPR_TIM2_REMAP;

    /* PB10 / PB11 = AF push-pull 50 MHz (CRH nibbles 8,12 → MODE=11 CNF=10 → 0xB). */
    GPIOB->CRH = (GPIOB->CRH & ~((0xFu << 8) | (0xFu << 12)))
               |  ((0xBu << 8) | (0xBu << 12));

    /* TIM2 base: 1 µs tick, 20 ms period. */
    TIM2->CR1   = 0;
    TIM2->PSC   = 64u - 1u;
    TIM2->ARR   = 20000u - 1u;

    /* CH3/CH4: PWM mode 1, preload enabled. CCMR2 covers OC3/OC4. */
    TIM2->CCMR2 = (6u << 4) | (1u << 3)        /* OC3M=PWM1, OC3PE=1 */
                | (6u << 12) | (1u << 11);     /* OC4M=PWM1, OC4PE=1 */
    TIM2->CCR3  = deg_to_us(90);
    TIM2->CCR4  = deg_to_us(90);
    TIM2->CCER  = TIM_CCER_CC3E | TIM_CCER_CC4E;

    TIM2->EGR = TIM_EGR_UG;
    TIM2->CR1 = TIM_CR1_ARPE | TIM_CR1_CEN;

    s_angle[0] = 90;
    s_angle[1] = 90;
}

void board_set_angle(uint8_t ch, uint8_t deg)
{
    if (ch > 1) return;
    if (deg > 180) deg = 180;
    s_angle[ch] = deg;
    uint16_t us = deg_to_us(deg);
    if (ch == 0) TIM2->CCR3 = us;
    else         TIM2->CCR4 = us;
}

uint8_t board_get_angle(uint8_t ch) { return ch < 2 ? s_angle[ch] : 0; }
uint8_t board_is_moving(void)        { return 0; }
