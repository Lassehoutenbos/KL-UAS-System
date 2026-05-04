#include "board.h"

#include "stm32f1xx.h"

/* LightBar on STM32F103C8T6 (KL-GCS-MODBUS03): WS2812B (LED1) on PB0.
   On this board only one LED is fitted, but the framework happily
   streams BOARD_NUM_PIXELS worth of bits — the extra bits clock past
   the last LED and are discarded.

   Bit-bang protocol — at 64 MHz SYSCLK, 1 cycle = 15.625 ns. WS2812B:
       T0H ≈ 0.35 µs ≈ 22 cycles
       T1H ≈ 0.70 µs ≈ 45 cycles
       period ≈ 1.25 µs ≈ 80 cycles
   Cycle-accurate timing is enforced via the DWT cycle counter and the
   send loop runs with interrupts disabled. */

#undef BOARD_PIN_LED
#define BOARD_PIN_LED   0u    /* PB0 (LED1) */

#define WS2812_T0H_CYC  22u
#define WS2812_T1H_CYC  45u
#define WS2812_BIT_CYC  80u

static uint8_t  s_brightness;
static uint8_t  s_mode;
static uint16_t s_colour;
static uint8_t  s_dirty;

static void dwt_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
}

static inline void ws2812_send_byte(uint8_t b)
{
    for (int i = 0; i < 8; i++) {
        uint32_t t = DWT->CYCCNT;
        GPIOB->BSRR = (1u << BOARD_PIN_LED);
        uint32_t hi = (b & 0x80) ? WS2812_T1H_CYC : WS2812_T0H_CYC;
        while ((DWT->CYCCNT - t) < hi) { }
        GPIOB->BSRR = (1u << (BOARD_PIN_LED + 16));
        while ((DWT->CYCCNT - t) < WS2812_BIT_CYC) { }
        b <<= 1;
    }
}

/* RGB16 565 → 24-bit GRB (WS2812 wire order). */
static void rgb16_to_grb(uint16_t c, uint8_t b, uint8_t out[3])
{
    uint8_t r5 = (c >> 11) & 0x1F;
    uint8_t g6 = (c >>  5) & 0x3F;
    uint8_t b5 =  c        & 0x1F;
    uint16_t r8 = (uint16_t)((r5 << 3) | (r5 >> 2));
    uint16_t g8 = (uint16_t)((g6 << 2) | (g6 >> 4));
    uint16_t b8 = (uint16_t)((b5 << 3) | (b5 >> 2));
    out[0] = (uint8_t)((g8 * b) / 255u);
    out[1] = (uint8_t)((r8 * b) / 255u);
    out[2] = (uint8_t)((b8 * b) / 255u);
}

static void ws2812_send_strip(uint16_t colour, uint8_t bright)
{
    uint8_t grb[3];
    rgb16_to_grb(colour, bright, grb);

    __disable_irq();
    for (int n = 0; n < BOARD_NUM_PIXELS; n++) {
        ws2812_send_byte(grb[0]);
        ws2812_send_byte(grb[1]);
        ws2812_send_byte(grb[2]);
    }
    __enable_irq();
    /* Latch: line low ≥50 µs handled naturally by main-loop cadence. */
}

void board_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    /* PB0 = output push-pull 50 MHz: CRL nibble at bit 0 → 0x3. */
    GPIOB->CRL = (GPIOB->CRL & ~(0xFu << 0)) | (0x3u << 0);
    GPIOB->BSRR = (1u << (BOARD_PIN_LED + 16));   /* idle low */

    dwt_init();
    s_dirty = 1;
}

void board_set_brightness(uint8_t b)        { s_brightness = b; s_dirty = 1; }
uint8_t board_get_brightness(void)          { return s_brightness; }
void board_set_mode(uint8_t m)              { s_mode = m;       s_dirty = 1; }
uint8_t board_get_mode(void)                { return s_mode; }
void board_set_colour_rgb16(uint16_t c)     { s_colour = c;     s_dirty = 1; }
uint16_t board_get_colour_rgb16(void)       { return s_colour; }

void board_tick(void)
{
    if (!s_dirty) return;
    s_dirty = 0;
    ws2812_send_strip(s_colour, s_brightness);
}

uint16_t board_estimate_power_mw(void)
{
    uint32_t mw = (uint32_t)BOARD_NUM_PIXELS * 60u * s_brightness / 255u;
    return mw > 0xFFFFu ? 0xFFFFu : (uint16_t)mw;
}
