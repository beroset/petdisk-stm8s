#ifndef TIMER1MS_H
#define TIMER1MS_H
#include "stm8.h"

void timer1ms_init();
void delay_ms(unsigned ms);
void delay_us(unsigned us);
void timer3_isr(void) __interrupt(TIM3_OVF_ISR);
#endif // TIMER1MS_H
