#ifndef GPIB_H
#define GPIB_H
#include <stdint.h>
#include "stm8.h"

/*
 * this ISR declaration needs to be here and this header included
 * in main.c; otherwise, SDCC will not put it into the vector table.
 */
void atn_isr(void) __interrupt(EXTI1_ISR);
void dav_isr(void) __interrupt(EXTI0_ISR);
void gpib_init();
uint8_t gpib_eoi();
uint8_t gpib_atn();
uint8_t gpib_dav();
uint8_t gpib_nrfd();
uint8_t gpib_ndac();
uint8_t gpib_data();
uint32_t gpib_intcount();
void gpib_printbuffer();
#endif // GPIB_H
