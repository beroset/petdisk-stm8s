#include <stdint.h>
#include <stdio.h>
#include "stm8.h"
#include "display.h"

#define LEDBIT 5

#if 0
unsigned char __sdcc_external_startup(void) {
    return 0;
}
#endif
static volatile bool ok = false;

void timer2_isr(void) __interrupt(13)
{
    ok = true;
    // clear IT pending bit
    TIM2_SR1 &= ~1;
    // toggle LED
    PC_ODR ^= (1 << LEDBIT);
}

const uint16_t reload_value = 15625;

void main(void)
{
    CLK_CKDIVR = 0x18; // Set the frequency to 2 MHz
    TIM2_PSCR = 0b00000111;  // prescaler = 128; T=64us
    TIM2_ARRH = reload_value >> 8;
    TIM2_ARRL = reload_value & 0x00ff;
    TIM2_IER = 0x01;  // update interrupt enable
    TIM2_CR1 = 0x01;  // enable timer

    // set up LED output
    PC_DDR = 1 << LEDBIT;
    PC_CR1 = 1 << LEDBIT;
    PC_CR2 = 0;

    // set PD0 also as output
    PD_DDR = 1;
    PD_CR1 = 1;
    PD_ODR = 1; // turn it on

    enableInterrupts();
    for (;;) {
        waitForInterrupt();
    }
}
