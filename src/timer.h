#ifndef TIMER1MS_H
#define TIMER1MS_H
#include "stm8.h"

void timer_init();
void delay_ms(unsigned ms);
/*
 * delay for microseconds 
 * should be in increments of 50us to be accurate
 */
#define delay_us(microseconds) \
    for (unsigned i = microseconds * 16 / 50; i; --i)  __asm__("nop")

void timer3_isr(void) __interrupt(TIM3_OVF_ISR);
#endif // TIMER1MS_H
