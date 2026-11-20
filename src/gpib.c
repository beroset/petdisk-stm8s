#include <stdint.h>
#include <stdio.h>
#include "gpib.h"
#include "stm8.h"
#include "pindefs.h"

static volatile uint32_t intcount = 0;
static uint8_t myaddr;
static enum bus_state state;

static const uint8_t LOAD = 0xF0;
static const uint8_t SAVE = 0xF1;
static volatile uint8_t current_cmd;

static volatile uint8_t myfilename[32];
static volatile uint8_t* fn;
static int (*callback)(int, int);


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
        uint8_t cmd = ~gpib_data();
        switch (state) {
            case idle:
                if (cmd == (0x20 | myaddr)) {
                    state = listening;
                }
                break;
            case listening:
                if (cmd == LOAD || cmd == SAVE) {
                    current_cmd = cmd;
                    fn = myfilename;
                    state = filename;
                } else {
                    // not for us
                    state = idle;
                }
                break;
            case filename:
                *fn++ = cmd;
                if (!eoi) {
                    *fn++ = '\0';
                    state = unlisten;
                }
                break;
            case unlisten:
                if (!atn && cmd == 0x3F) { // unlisten
                    state = lagsag;
                } else {
                    state = idle;  // error
                }
                break;
            case lagsag:
                if (!atn
                    && ((current_cmd == SAVE && cmd == (0x20 | myaddr))
                    || (current_cmd == LOAD && cmd == (0x40 | myaddr))
                    ))
                {
                    state = scg;
                } else {
                    state = idle;  // error
                }
                break;
            case scg:
                if (atn) {
                    state = idle; // error
                } else if (current_cmd == SAVE && cmd == 0x61) {
                    state = save;

                } else if (current_cmd == LOAD && cmd == 0x60) {
                    state = load;
                } else {
                    state = idle; // error
                }
                break;
            case load:
                break;
            case save:
                callback(cmd, 1);
                if (!eoi) {
                    state = idle;
                }
                break;
        }
        SET(NDAC);
    } else {
        CLR(NDAC);
        SET(NRFD);
    }
}

void gpib_init()
{
    // read my device address from EEPROM
    myaddr = EEPROM;
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
uint8_t gpib_address() { return myaddr; }

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

enum bus_state gpib_state()
{
    return state;
}

const char *gpib_filename()
{
    return myfilename;
}

void gpib_setcallback(int (*cb)(int, int))
{
    callback = cb;
}
