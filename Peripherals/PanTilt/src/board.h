#ifndef PANTILT_BOARD_H
#define PANTILT_BOARD_H

#include <stdint.h>

#define BOARD_ADDR        0x03

/* Two servo PWM outputs. RP2040 default — STM32/ESP32 ports remap. */
#define BOARD_PIN_SERVO_PAN   14
#define BOARD_PIN_SERVO_TILT  15

void    board_init(void);
void    board_set_angle(uint8_t channel, uint8_t deg);   /* 0..180 */
uint8_t board_get_angle(uint8_t channel);
uint8_t board_is_moving(void);

#endif
