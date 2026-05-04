#include "hal.h"

#include "stm32f1xx.h"

/* ------------------------------------------------------------------ */
/* Pin map — defaults match KL-GCS-MODBUS03 PCB1 (STM32F103C8T6).
   USART2 PA2/PA3 to RS-485 transceiver, DE on PA1, /INT on PA4.
   Override at build time with -DHAL_STM32F1_PIN_… for other boards.   */
/* ------------------------------------------------------------------ */
#ifndef HAL_STM32F1_USART
#define HAL_STM32F1_USART       USART2
#endif
#ifndef HAL_STM32F1_USART_APB1EN
#define HAL_STM32F1_USART_APB1EN  RCC_APB1ENR_USART2EN
#endif
#ifndef HAL_STM32F1_GPIO_PORT
#define HAL_STM32F1_GPIO_PORT   GPIOA
#endif
#ifndef HAL_STM32F1_GPIO_RCC
#define HAL_STM32F1_GPIO_RCC    RCC_APB2ENR_IOPAEN
#endif
#ifndef HAL_STM32F1_PIN_TX
#define HAL_STM32F1_PIN_TX      2u    /* PA2  USART2_TX */
#endif
#ifndef HAL_STM32F1_PIN_RX
#define HAL_STM32F1_PIN_RX      3u    /* PA3  USART2_RX */
#endif
#ifndef HAL_STM32F1_PIN_DE
#define HAL_STM32F1_PIN_DE      1u    /* PA1  RS-485 DE */
#endif
#ifndef HAL_STM32F1_PIN_INT
#define HAL_STM32F1_PIN_INT     4u    /* PA4  /INT to master */
#endif

/* APB1 PCLK after our HSI->PLL setup. USART2 lives on APB1. */
#define HAL_STM32F1_SYSCLK      64000000u
#define HAL_STM32F1_PCLK1       32000000u

/* ------------------------------------------------------------------ */
/* Clock setup — HSI/2 * 16 = 64 MHz on the F103.
   The KL-GCS-MODBUS03 board has no HSE crystal, so we drive PLL from
   HSI and ignore the HSE-centric stock SystemInit().                   */
/* ------------------------------------------------------------------ */

static void clock_init_hsi_pll_64mhz(void)
{
    /* HSI is on after reset; wait for ready as a sanity check. */
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY)) { }

    /* Switch SYSCLK to HSI while we reconfigure the PLL. */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_HSI;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI) { }

    /* Two flash wait states are required above 48 MHz. */
    FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2;

    RCC->CR &= ~RCC_CR_PLLON;
    while (RCC->CR & RCC_CR_PLLRDY) { }

    /* PLLSRC = HSI/2, PLLMUL = x16 -> 4 MHz * 16 = 64 MHz.
       AHB /1, APB1 /2 (max 36 MHz), APB2 /1, ADC /8 (8 MHz). */
    uint32_t cfgr = RCC->CFGR;
    cfgr &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL
            | RCC_CFGR_HPRE   | RCC_CFGR_PPRE1    | RCC_CFGR_PPRE2
            | RCC_CFGR_ADCPRE);
    cfgr |= RCC_CFGR_PLLMULL16
         |  RCC_CFGR_HPRE_DIV1
         |  RCC_CFGR_PPRE1_DIV2
         |  RCC_CFGR_PPRE2_DIV1
         |  RCC_CFGR_ADCPRE_DIV8;
    RCC->CFGR = cfgr;

    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) { }

    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) { }

    SystemCoreClock = HAL_STM32F1_SYSCLK;
}

/* ------------------------------------------------------------------ */
/* SysTick — 1 kHz millisecond tick                                     */
/* ------------------------------------------------------------------ */

static volatile uint32_t s_ticks;

void SysTick_Handler(void)
{
    s_ticks++;
}

uint32_t hal_millis(void)
{
    return s_ticks;
}

/* ------------------------------------------------------------------ */
/* GPIO helpers — F1 uses CRL/CRH (4 bits/pin). Each pin's nibble:
       MODE[1:0] : 00=input, 01=10MHz out, 10=2MHz out, 11=50MHz out
       CNF [1:0] : (out)  00=push-pull, 01=open-drain,
                          10=AF push-pull, 11=AF open-drain
                   (in)   00=analog, 01=floating, 10=pull-up/down       */
/* ------------------------------------------------------------------ */

#define GPIO_CFG_IN_FLOAT       0x4u   /* MODE=00 CNF=01 */
#define GPIO_CFG_OUT_PP_50      0x3u   /* MODE=11 CNF=00 */
#define GPIO_CFG_OUT_OD_50      0x7u   /* MODE=11 CNF=01 */
#define GPIO_CFG_AF_PP_50       0xBu   /* MODE=11 CNF=10 */

static void gpio_set_cfg(GPIO_TypeDef *p, uint32_t pin, uint32_t cfg)
{
    if (pin < 8u) {
        uint32_t s = pin * 4u;
        p->CRL = (p->CRL & ~(0xFu << s)) | (cfg << s);
    } else {
        uint32_t s = (pin - 8u) * 4u;
        p->CRH = (p->CRH & ~(0xFu << s)) | (cfg << s);
    }
}

/* ------------------------------------------------------------------ */
/* HAL                                                                  */
/* ------------------------------------------------------------------ */

void hal_uart_init(uint32_t baud)
{
    clock_init_hsi_pll_64mhz();

    /* Clocks: GPIO port + USART (USART2 is on APB1). */
    RCC->APB2ENR |= HAL_STM32F1_GPIO_RCC;
    RCC->APB1ENR |= HAL_STM32F1_USART_APB1EN;

    /* TX = AF push-pull 50MHz, RX = floating input. */
    gpio_set_cfg(HAL_STM32F1_GPIO_PORT, HAL_STM32F1_PIN_TX, GPIO_CFG_AF_PP_50);
    gpio_set_cfg(HAL_STM32F1_GPIO_PORT, HAL_STM32F1_PIN_RX, GPIO_CFG_IN_FLOAT);

    /* DE = output push-pull, idle low (receive). */
    gpio_set_cfg(HAL_STM32F1_GPIO_PORT, HAL_STM32F1_PIN_DE, GPIO_CFG_OUT_PP_50);
    HAL_STM32F1_GPIO_PORT->BSRR = (1u << (HAL_STM32F1_PIN_DE + 16));

    /* USART: 8N1, TE+RE+UE. BRR = PCLK / baud (oversample 16). */
    HAL_STM32F1_USART->CR1 = 0;
    HAL_STM32F1_USART->CR2 = 0;
    HAL_STM32F1_USART->CR3 = 0;
    HAL_STM32F1_USART->BRR = HAL_STM32F1_PCLK1 / baud;
    HAL_STM32F1_USART->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;

    /* SysTick at 1 kHz from SYSCLK. */
    SysTick_Config(HAL_STM32F1_SYSCLK / 1000u);
}

void hal_uart_set_tx_enable(bool enable)
{
    if (enable) HAL_STM32F1_GPIO_PORT->BSRR = (1u << HAL_STM32F1_PIN_DE);
    else        HAL_STM32F1_GPIO_PORT->BSRR = (1u << (HAL_STM32F1_PIN_DE + 16));
}

void hal_uart_write(const uint8_t *buf, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        while (!(HAL_STM32F1_USART->SR & USART_SR_TXE)) { }
        HAL_STM32F1_USART->DR = buf[i];
    }
    /* Wait for the last byte to fully clock out before releasing DE. */
    while (!(HAL_STM32F1_USART->SR & USART_SR_TC)) { }
}

int hal_uart_read(uint8_t *byte_out)
{
    if (!(HAL_STM32F1_USART->SR & USART_SR_RXNE)) return 0;
    *byte_out = (uint8_t)HAL_STM32F1_USART->DR;
    return 1;
}

void hal_int_pin_init(void)
{
    RCC->APB2ENR |= HAL_STM32F1_GPIO_RCC;
    /* Open-drain output, idle high (released). */
    HAL_STM32F1_GPIO_PORT->BSRR = (1u << HAL_STM32F1_PIN_INT);
    gpio_set_cfg(HAL_STM32F1_GPIO_PORT, HAL_STM32F1_PIN_INT, GPIO_CFG_OUT_OD_50);
}

void hal_int_pin_drive(bool assert_low)
{
    if (assert_low)
        HAL_STM32F1_GPIO_PORT->BSRR = (1u << (HAL_STM32F1_PIN_INT + 16));
    else
        HAL_STM32F1_GPIO_PORT->BSRR = (1u << HAL_STM32F1_PIN_INT);
}
