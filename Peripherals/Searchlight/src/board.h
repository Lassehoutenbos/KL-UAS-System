#ifndef SEARCHLIGHT_BOARD_H
#define SEARCHLIGHT_BOARD_H

#include <stdint.h>
#include <stdbool.h>

/* Hardware-agnostic board API. Implementations live in
   src/board_<mcu>.c and are selected by the app's CMakeLists.txt. */

#define BOARD_ADDR        0x01

/* Searchlight has one PWM-driven LED + an NTC for thermal monitoring. */
#define BOARD_PIN_PWM     15      /* MOSFET gate */
#define BOARD_PIN_NTC_ADC 26      /* GP26 = ADC0 */

/* Fault flag bits (matches RS485_PERIPHERAL_BUS.md §Searchlight) */
#define FAULT_OVERCURRENT     (1u << 0)
#define FAULT_OVERTEMP        (1u << 1)
#define FAULT_VBAT_UV         (1u << 2)

/* Thermal shutoff threshold; can be overridden at runtime via SET_PARAM. */
#define BOARD_OVERTEMP_C      80

void    board_init(void);
void    board_set_pwm(uint8_t duty);
uint8_t board_get_pwm(void);
int8_t  board_read_temp_c(void);
uint8_t board_fault_flags(void);
bool    board_overtemp(void);

#endif
