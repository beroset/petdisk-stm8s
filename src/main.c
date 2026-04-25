#include <stdint.h>
#include <stdio.h>
#include "stm8.h"
#include "display.h"
#include "timer1ms.h"

#define LEDBIT 5
#define CLOCKBIT 7

#if 0
unsigned char __sdcc_external_startup(void) {
    return 0;
}
#endif

void timer2_isr(void) __interrupt(TIM2_OVF_ISR)
{
    // clear IT pending bit
    TIM2_SR1 &= ~1;
    // toggle LED
    PC_ODR ^= (1u << LEDBIT);
}

static void init()
{
    const uint16_t tim2_reload_value = 15625;

    CLK_CKDIVR = 0x18; // Set the frequency to 2 MHz; T=0.5us
    TIM2_PSCR = 0b00000111;  // prescaler = 7; T=64us
    TIM2_ARRH = tim2_reload_value >> 8;
    TIM2_ARRL = tim2_reload_value & 0x00ff;
    TIM2_IER = 0x01;  // update interrupt enable
    TIM2_CR1 = 0x01;  // enable timer

    timer1ms_init();

    // set up LED output
    PC_DDR = 1u << LEDBIT;
    PC_CR1 = 1u << LEDBIT;
    PC_CR2 = 0;

    // set PD7 also as output
    PD_DDR = 1u << CLOCKBIT;
    PD_CR1 = 1u << CLOCKBIT;
    PD_ODR = 1u << CLOCKBIT;

}

void main(void)
{
    init();
    enableInterrupts();
    for (;;) {
        delay_ms(3);  // wait 3ms
        PD_ODR ^= (1 << CLOCKBIT); // toggle pin
        waitForInterrupt();
    }
}
