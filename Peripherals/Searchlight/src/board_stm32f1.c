#include "board.h"

#include "stm32f1xx.h"

/* Searchlight on STM32F103C8T6 (KL-GCS-MODBUS03):
   - PWM gate drive: TIM1_CH1N on PB13 (J7 expansion header).
     Complementary output is used as a regular PWM output by enabling
     CC1NE and BDTR.MOE; CC1NP=0 makes CH1N follow the OC1REF waveform.
   - NTC sense:      ADC1_IN0 on PA0 (analog input). PA0 is unused in
     the netlist so the NTC divider must be wired here.

   Override at compile time with -DBOARD_PWM_TIM=… etc. if needed.    */

static uint8_t s_duty;

static uint16_t adc_read_in0(void)
{
    ADC1->SQR3 = 0u;                    /* single conversion, channel 0 */
    ADC1->CR2 |= ADC_CR2_ADON;          /* start conversion */
    while (!(ADC1->SR & ADC_SR_EOC)) { }
    return (uint16_t)ADC1->DR;
}

void board_init(void)
{
    /* Clocks: GPIOA (ADC pin), GPIOB (PWM pin), AFIO, TIM1, ADC1. */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN
                  | RCC_APB2ENR_AFIOEN | RCC_APB2ENR_TIM1EN
                  | RCC_APB2ENR_ADC1EN;

    /* PB13 = AF push-pull 50 MHz. CRH bit field for pin 13 starts at (13-8)*4=20. */
    GPIOB->CRH = (GPIOB->CRH & ~(0xFu << 20)) | (0xBu << 20);

    /* PA0 = analog input. CRL bits [3:0] = 0000. */
    GPIOA->CRL &= ~(0xFu << 0);

    /* TIM1 PWM @ ~3.9 kHz: TIMxCLK = 64 MHz, PSC=63 → 1 MHz tick,
       ARR=255 → 256 µs period (~3906 Hz), 8-bit duty resolution. */
    TIM1->CR1   = 0;
    TIM1->PSC   = 64u - 1u;
    TIM1->ARR   = 256u - 1u;
    TIM1->CCMR1 = (6u << 4) | TIM_CCMR1_OC1PE;     /* OC1M=PWM1, preload */
    TIM1->CCR1  = 0;
    TIM1->CCER  = TIM_CCER_CC1NE;                  /* enable CH1N output */
    TIM1->BDTR  = TIM_BDTR_MOE;                    /* main output enable */
    TIM1->EGR   = TIM_EGR_UG;
    TIM1->CR1   = TIM_CR1_ARPE | TIM_CR1_CEN;

    /* ADC1: single conversion, sw start. ADCCLK = PCLK2/8 = 8 MHz. */
    ADC1->CR1   = 0;
    ADC1->CR2   = ADC_CR2_ADON;                    /* power up */
    /* per RM, brief delay then ADON again to actually start conversions;
       a couple dummy reads suffice. */
    for (volatile int i = 0; i < 1000; i++) { }
    ADC1->SMPR2 = 7u << 0;                         /* ch0: 239.5 cycles */
    ADC1->SQR1  = 0u;                              /* L=0 → 1 conversion */
}

void board_set_pwm(uint8_t duty)
{
    s_duty = duty;
    TIM1->CCR1 = duty;
}

uint8_t board_get_pwm(void) { return s_duty; }

int8_t board_read_temp_c(void)
{
    uint16_t raw = adc_read_in0();      /* 0..4095 */
    /* Same linearisation the rp2040 board uses: 25 °C centered at mid-scale. */
    int32_t c = 25 + ((2048 - (int32_t)raw) * 60) / 2048;
    if (c < -40) c = -40;
    if (c > 125) c = 125;
    return (int8_t)c;
}

bool    board_overtemp(void)    { return board_read_temp_c() >= BOARD_OVERTEMP_C; }
uint8_t board_fault_flags(void) { return board_overtemp() ? FAULT_OVERTEMP : 0; }
