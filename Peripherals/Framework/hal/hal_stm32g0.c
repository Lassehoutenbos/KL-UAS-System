#include "hal.h"

#include "stm32g0xx.h"

/* ------------------------------------------------------------------ */
/* Pin map (override at build time with -D… if a board uses different
   pins; defaults are the most common bring-up pins on a Nucleo G031K8). */
/* ------------------------------------------------------------------ */
#ifndef HAL_STM32_USART
#define HAL_STM32_USART     USART1
#endif
#ifndef HAL_STM32_USART_RCC_BIT
#define HAL_STM32_USART_RCC_BIT  RCC_APBENR2_USART1EN
#endif
#ifndef HAL_STM32_USART_AF
#define HAL_STM32_USART_AF  1u            /* AF1 = USART1 on PA9/PA10 */
#endif
#ifndef HAL_STM32_GPIO_PORT
#define HAL_STM32_GPIO_PORT GPIOA
#endif
#ifndef HAL_STM32_PIN_TX
#define HAL_STM32_PIN_TX    9u            /* PA9 */
#endif
#ifndef HAL_STM32_PIN_RX
#define HAL_STM32_PIN_RX    10u           /* PA10 */
#endif
#ifndef HAL_STM32_PIN_DE
#define HAL_STM32_PIN_DE    8u            /* PA8 */
#endif
#ifndef HAL_STM32_PIN_INT
#define HAL_STM32_PIN_INT   7u            /* PA7 */
#endif

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
/* GPIO helpers (no LL — direct register writes keep the dep surface
/* small and the binary tiny).                                          */
/* ------------------------------------------------------------------ */

static void gpio_mode(GPIO_TypeDef *p, uint32_t pin, uint32_t mode)
{
    /* mode: 0 input, 1 output, 2 alt, 3 analog */
    p->MODER = (p->MODER & ~(3u << (pin * 2))) | (mode << (pin * 2));
}
static void gpio_otype_od(GPIO_TypeDef *p, uint32_t pin, int open_drain)
{
    if (open_drain) p->OTYPER |=  (1u << pin);
    else            p->OTYPER &= ~(1u << pin);
}
static void gpio_set_af(GPIO_TypeDef *p, uint32_t pin, uint32_t af)
{
    uint32_t idx = pin >> 3;
    uint32_t shift = (pin & 7u) * 4u;
    p->AFR[idx] = (p->AFR[idx] & ~(0xFu << shift)) | (af << shift);
}

/* ------------------------------------------------------------------ */
/* HAL                                                                  */
/* ------------------------------------------------------------------ */

void hal_uart_init(uint32_t baud)
{
    /* Clocks */
    RCC->IOPENR  |= RCC_IOPENR_GPIOAEN;
    RCC->APBENR2 |= HAL_STM32_USART_RCC_BIT;

    /* TX/RX = AF, DE = output push-pull */
    gpio_mode(HAL_STM32_GPIO_PORT, HAL_STM32_PIN_TX, 2);
    gpio_set_af(HAL_STM32_GPIO_PORT, HAL_STM32_PIN_TX, HAL_STM32_USART_AF);
    gpio_mode(HAL_STM32_GPIO_PORT, HAL_STM32_PIN_RX, 2);
    gpio_set_af(HAL_STM32_GPIO_PORT, HAL_STM32_PIN_RX, HAL_STM32_USART_AF);

    gpio_mode(HAL_STM32_GPIO_PORT, HAL_STM32_PIN_DE, 1);
    HAL_STM32_GPIO_PORT->BSRR = (1u << (HAL_STM32_PIN_DE + 16));   /* low */

    /* USART: assume PCLK = SystemCoreClock. Caller's startup must have
       set SystemCoreClock; default after reset on G031 is 16 MHz HSI. */
    extern uint32_t SystemCoreClock;
    HAL_STM32_USART->CR1 = 0;
    HAL_STM32_USART->BRR = SystemCoreClock / baud;
    HAL_STM32_USART->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;

    /* SysTick at 1 kHz */
    SysTick_Config(SystemCoreClock / 1000u);
}

void hal_uart_set_tx_enable(bool enable)
{
    if (enable) HAL_STM32_GPIO_PORT->BSRR = (1u << HAL_STM32_PIN_DE);
    else        HAL_STM32_GPIO_PORT->BSRR = (1u << (HAL_STM32_PIN_DE + 16));
}

void hal_uart_write(const uint8_t *buf, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        while (!(HAL_STM32_USART->ISR & USART_ISR_TXE_TXFNF)) { }
        HAL_STM32_USART->TDR = buf[i];
    }
    /* Wait for the last byte to fully clock out before releasing DE. */
    while (!(HAL_STM32_USART->ISR & USART_ISR_TC)) { }
}

int hal_uart_read(uint8_t *byte_out)
{
    if (!(HAL_STM32_USART->ISR & USART_ISR_RXNE_RXFNE)) return 0;
    *byte_out = (uint8_t)HAL_STM32_USART->RDR;
    return 1;
}

void hal_int_pin_init(void)
{
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN;
    /* Open-drain output, idle high (line released — pulled high externally) */
    gpio_otype_od(HAL_STM32_GPIO_PORT, HAL_STM32_PIN_INT, 1);
    HAL_STM32_GPIO_PORT->BSRR = (1u << HAL_STM32_PIN_INT);   /* release */
    gpio_mode(HAL_STM32_GPIO_PORT, HAL_STM32_PIN_INT, 1);
}

void hal_int_pin_drive(bool assert_low)
{
    if (assert_low)
        HAL_STM32_GPIO_PORT->BSRR = (1u << (HAL_STM32_PIN_INT + 16));
    else
        HAL_STM32_GPIO_PORT->BSRR = (1u << HAL_STM32_PIN_INT);
}
