#include <stdint.h>
#include <stdio.h>
#include "gpib.h"
#include "stm8.h"
#include "pindefs.h"

static volatile uint32_t intcount = 0;

void atn_isr(void) __interrupt(EXTI1_ISR)
{
    FLIP(BLUE_LED);
    ++intcount;
}

void gpib_init()
{
    // configure GPIB 8-bit data as input
    IN_SPAN(GPIBLO);
    IN_SPAN(GPIBHI);
    IN(ATN);
    CCR1(ATN); // floating
    SCR2(ATN); // enable interrupt
    EXTI_CR1 = (EXTI_CR1 & ~0x0C) | 0x0C;  // falling or rising edge
    IN(EOI);
    CCR1(EOI); // floating
    /*
     * These change depending on whether we're Talker or Listener.
     * Assume Listener initially.
     */
    IN(DAV);
    CCR1(DAV); // floating
    CCR2(DAV); // without interrupt
#if 0
    OUT(NRFD);  // open drain output
    CCR1(NRFD);
    CCR2(NRFD);
    OUT(NDAC);  // open drain output
    CCR1(NDAC);
    CCR2(NDAC);
#endif
    // set up LED output
    OUT(BLUE_LED);
    SCR1(BLUE_LED);
    CCR2(BLUE_LED);
    puts("blue LED set up");
}

uint8_t gpib_eoi() { return READ(EOI); }
uint8_t gpib_atn() { return READ(ATN); }
uint8_t gpib_dav() { return READ(DAV); }
uint8_t gpib_nrfd() { return READ(NRFD); }
uint8_t gpib_ndac() { return READ(NDAC); }

uint8_t gpib_data()
{
    uint8_t lo = READ_SPAN(GPIBLO);
    uint8_t hi = READ_SPAN(GPIBHI);
    return (hi << 5) | lo;
}

uint32_t gpib_intcount()
{
    uint32_t count;
    disableInterrupts();
    count = intcount;
    enableInterrupts();
    return count;
}
