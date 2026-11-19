#include <stdint.h>
#include <stdio.h>
#include "gpib.h"
#include "stm8.h"
#include "pindefs.h"

static volatile uint32_t intcount = 0;
static volatile uint8_t data[256];
static volatile uint8_t flags[256];
static volatile uint8_t write_location;
static uint8_t read_location;

void atn_isr(void) __interrupt(EXTI1_ISR)
{
    SET(NRFD);
    CLR(NDAC);
    FLIP(BLUE_LED);
    ++intcount;
}

void dav_isr(void) __interrupt(EXTI0_ISR)
{
    uint8_t atn = READ(ATN);
    uint8_t eoi = READ(EOI);
    if (READ(DAV) == 0) {
        CLR(NRFD);
        data[write_location] = ~gpib_data();
        flags[write_location++] = (atn ? 0 : 1) | (eoi ? 0 : 2);
        SET(NDAC);
    } else {
        CLR(NDAC);
        SET(NRFD);
    }
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
    SCR2(DAV); // with interrupt
    EXTI_CR1 = (EXTI_CR1 & ~0x03) | 0x03;  // falling or rising edge

    OUT(NRFD);  // open drain output
    CCR1(NRFD);
    CCR2(NRFD);
    SET(NRFD);

    OUT(NDAC);  // open drain output
    CCR1(NDAC);
    CCR2(NDAC);
    CLR(NDAC);

    // set up LED output
    OUT(BLUE_LED);
    SCR1(BLUE_LED);
    CCR2(BLUE_LED);
}

uint8_t gpib_eoi() { return READ(EOI); }
uint8_t gpib_atn() { return READ(ATN); }
uint8_t gpib_dav() { return READ(DAV); }
uint8_t gpib_nrfd() { return READ(NRFD); }
uint8_t gpib_ndac() { return READ(NDAC); }

uint8_t gpib_data()
{
    uint8_t val = READ_SPAN(GPIBLO);
    if (val & 0x80) val ^= 0x82;
    val |= READ_SPAN(GPIBHI);
    return val;
}

uint32_t gpib_intcount()
{
    uint32_t count;
    disableInterrupts();
    count = intcount;
    enableInterrupts();
    return count;
}

void gpib_printbuffer()
{
    puts("data:");
    for ( ; read_location != write_location; ++read_location) {
        printf(" %x %02x", flags[read_location], data[read_location]);
    }
    putchar('\n');
}
