#include <stdint.h>
#include "timer1ms.h"
#include "stm8.h"

static volatile uint32_t ticks = 0;

void timer3_isr(void) __interrupt(TIM3_OVF_ISR)
{
    // clear IT pending bit
    BITCLR(TIM3_SR1, 0);
    ++ticks;
}

void timer1ms_init()
{
    const uint16_t tim3_reload_value = 125;

    TIM3_PSCR = 0b00000100;  // prescaler = 4; T=2us
    TIM3_ARRH = tim3_reload_value >> 8;
    TIM3_ARRL = tim3_reload_value & 0x00ff;
    TIM3_IER = 0x01;  // update interrupt enable
    TIM3_CR1 = 0x01;  // enable timer
}

void delay_ms(unsigned ms)
{
    for (uint32_t start = ticks-1; ticks - start < ms; ) {
        waitForInterrupt();
    }
}

void delay_us(unsigned us)
{
    static const unsigned nop_delay = 2;
    for (unsigned i = us * nop_delay; i; --i)
        __asm__("nop");
}
