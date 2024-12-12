#ifndef __electric_throttle__
#define __electric_throttle__

#include "driverlib.h"
#include "uart.h"

extern volatile uint8_t new_throttle_val;
extern volatile  uint16_t throttle_val;
extern volatile  uint16_t max_throttle_adc;
extern volatile  uint16_t min_throttle_adc;
extern uint8_t throttle_en;

void adc_init();
void throttle_en_btn_init();
inline uint8_t is_throttle_en_btn_press();

#endif

