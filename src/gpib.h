#ifndef GPIB_H
#define GPIB_H
#include "stm8.h"

void gpib_init();
uint8_t gpib_eoi();
uint8_t gpib_atn();
uint8_t gpib_dav();
uint8_t gpib_nrfd();
uint8_t gpib_ndac();
uint8_t gpib_data();
//void timer3_isr(void) __interrupt(TIM3_OVF_ISR);
#endif // GPIB_H
