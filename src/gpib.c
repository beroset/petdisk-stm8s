#include <stdint.h>
#include "gpib.h"
#include "stm8.h"
#include "pindefs.h"

#if 0
static volatile uint32_t ticks = 0;

void timer3_isr(void) __interrupt(TIM3_OVF_ISR)
{
    // clear IT pending bit
    BITCLR(TIM3_SR1, 0);
    ++ticks;
}
#endif

void gpib_init()
{
    // configure GPIB 8-bit data as input
    IN_SPAN(GPIBLO);
    IN_SPAN(GPIBHI);
    IN(ATN);
    CCR1(ATN); // floating
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
