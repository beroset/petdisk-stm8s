#include <stdint.h>
#include <stdio.h>
#include "gpib.h"
#include "stm8.h"
#include "pindefs.h"

static volatile uint32_t intcount = 0;
static uint8_t myaddr;
static enum bus_state state;
static bool talking = false;
static volatile bool acked = false;

static const uint8_t LOAD = 0xF0;
static const uint8_t SAVE = 0xF1;
static volatile uint8_t current_cmd;

static volatile uint8_t myfilename[32];
static volatile uint8_t* fn;
static int (*save_callback)(int);

void atn_isr(void) __interrupt(EXTI1_ISR)
{
    SET(NRFD);
    CLR(NDAC);
    ++intcount;
}

void dav_isr(void) __interrupt(EXTI0_ISR)
{
    uint8_t atn = READ(ATN);
    uint8_t eoi = READ(EOI);

    if (READ(DAV) == 0) {
        CLR(NRFD);
        uint8_t cmd = ~gpib_dataread();
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
            case save:
                save_callback(cmd);
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

void ndac_isr(void) __interrupt(EXTI2_ISR)
{
    // Rising edge: PET deasserts NDAC (HIGH) = data accepted.
    CLR(DAV);
    FLIP(BLUE_LED);
    acked = true;
}

void gpib_init()
{
    // read my device address from EEPROM
    myaddr = EEPROM;

    IN(ATN);
    CCR1(ATN); // floating
    SCR2(ATN);
    EXTI_CR1 = (EXTI_CR1 & ~0x0C) | 0x0C;  // falling or rising edge
    // set up LED output
    OUT(BLUE_LED);
    SCR1(BLUE_LED);
    CCR2(BLUE_LED);

    gpib_set_listener_mode();
}

static void gpib_set_talker_mode()
{
    puts("entering talker mode");
    talking = true;
    disableInterrupts();
    // DAV: input -> output, drive low
    // Must drive DAV low explicitly: when switching from input (floating high)
    // to output, ODR state is unknown. PET may see a false DAV rising edge
    // and miss the data or read garbage.
    OUT(DAV);
    SET(DAV);
    SCR1(DAV);
    CCR2(DAV);

    // EOI: output -> output, drive low
    // Talker drives EOI to signal last byte
    OUT(EOI);
    CLR(EOI);
    SCR1(EOI);
    CCR2(EOI);

    // GPIB data: input -> output, drive all low first
    // Prevent bus contention during transition by driving data low
    // before switching direction.
    WRITE_SPAN(GPIBHI, 0);
    WRITE_SPAN(GPIBLO, 0);
    OUT_SPAN(GPIBLO);
    OUT_SPAN(GPIBHI);
    // note that because the reset value for PD_CR1 is 0x02, we
    // need to explicitly set the value so bit 1 works correctly
    CCR1_SPAN(GPIBLO);

    // NRFD: output high -> input
    IN(NRFD);
    CCR1(NRFD);

    // NDAC: output -> input, enable rising-edge interrupt
    IN(NDAC);
    CCR1(NDAC);
#if 0
    SCR2(NDAC);
#endif
    EXTI_CR1 = (EXTI_CR1 & ~0x30) | 0x10;  // PC2 rising edge only

    // Keep DAV interrupt enabled to detect UNT from PET during talker mode.
    // The ISR guards against interfering with the data handshake.

    // Disable EOI interrupt
    CCR2(EOI);
    enableInterrupts();
}

void gpib_set_listener_mode()
{
    talking = false;
    state = idle;

    // DAV: output -> input, enable interrupt
    IN(DAV);
    CCR1(DAV);
#if 1
    SCR2(DAV);
    EXTI_CR1 = (EXTI_CR1 & ~0x03) | 0x03;  // rising or falling edge
#endif

    // EOI: output -> input
    IN(EOI);
    CCR1(EOI);

    // GPIB data: output -> input
    IN_SPAN(GPIBLO);
    IN_SPAN(GPIBHI);

    // NRFD: input -> output high
    OUT(NRFD);
    CCR1(NRFD);
    CCR2(NRFD);
    SET(NRFD);

    // NDAC: input -> output low
    OUT(NDAC);
    CCR1(NDAC);
    CCR2(NDAC);
    CLR(NDAC);
}

uint8_t gpib_dataread()
{
    uint8_t val = READ_SPAN(GPIBLO);
    if (val & 0x80) val ^= 0x82;
    val |= READ_SPAN(GPIBHI);
    return val;
}

static void gpib_datawrite(uint8_t val)
{
    WRITE_SPAN(GPIBHI, val);
    if (val & 0x02) val ^= 0x82;
    WRITE_SPAN(GPIBLO, val);
}

uint32_t gpib_intcount()
{
    uint32_t count;
    disableInterrupts();
    count = intcount;
    enableInterrupts();
    return count;
}

void gpib_send_byte_talker(uint8_t byte, uint8_t eoi)
{
    if (!talking) {
        gpib_set_talker_mode();
    }
    acked = false;
    // Put byte on GPIB data bus
    gpib_datawrite(~byte);

    if (eoi) {
        CLR(EOI);
    } else {
        SET(EOI);
    }
    printf("Sending %02x %s\n", byte, (eoi ? "EOI" : ""));

    // Deassert DAV — signals listener that new data is on bus
#if 0
    CLR(DAV);
#else
#endif
    // gpib_dump();
    uint32_t timeout;
    const uint32_t MAXTIME = 50000;
    for (timeout=MAXTIME; timeout; --timeout) {
        bool nrfd = READ(NRFD);
        if (nrfd) break;
    }
    if (!timeout) {
        puts("NRFD high timeout");
    }
    CLR(DAV);
    for (timeout=MAXTIME; timeout; --timeout) {
        bool nrfd = READ(NRFD);
        if (!nrfd) break;
    }
    if (!timeout) {
        puts("NRFD low timeout");
    }
    for (timeout=MAXTIME; timeout; --timeout) {
        bool ndac = READ(NDAC);
        if (ndac) break;
    }
    if (!timeout) {
        puts("NDAC high timeout");
    }
    SET(DAV);
    for (timeout=MAXTIME; timeout; --timeout) {
        bool ndac = READ(NDAC);
        if (!ndac) break;
    }
    if (!timeout) {
        puts("NDAC low timeout");
    }
    SET(DAV);
    acked = true;
}


uint8_t gpib_eoi() { return READ(EOI); }
uint8_t gpib_atn() { return READ(ATN); }
uint8_t gpib_dav() { return READ(DAV); }
uint8_t gpib_nrfd() { return READ(NRFD); }
uint8_t gpib_ndac() { return READ(NDAC); }
uint8_t gpib_address() { return myaddr; }

bool gpib_acked() { return acked; }

enum bus_state gpib_state()
{
    return state;
}

const char *gpib_filename()
{
    return myfilename;
}

void gpib_set_save_callback(int (*cb)(int))
{
    save_callback = cb;
}

void gpib_dump()
{
    static const char* statenames[] = {
                "idle",
                "listening",
                "filename",
                "unlisten",
                "lagsag",
                "scg",
                "load",
                "save"
    };

    printf("EOI ATN DAV NRFD NDAC  data  state\n"
           " %u   %u   %u    %u    %u    %02x   %s\n",
           gpib_eoi() != 0,
           gpib_atn() != 0,
           gpib_dav() != 0,
           gpib_nrfd() != 0,
           gpib_ndac() != 0,
           ~gpib_dataread() & 0xff,
           statenames[gpib_state()]);
}

bool gpib_data_dump(uint8_t byte) {
    static bool dumptoggle = true;
    if (dumptoggle) {
        WRITE_SPAN(GPIBHI, 0);
        WRITE_SPAN(GPIBLO, 0);
        OUT_SPAN(GPIBLO);
        OUT_SPAN(GPIBHI);
        gpib_datawrite(~byte);

    } else {
        WRITE_SPAN(GPIBHI, 0);
        WRITE_SPAN(GPIBLO, 0);
        IN_SPAN(GPIBLO);
        IN_SPAN(GPIBHI);
    }
    dumptoggle = !dumptoggle;
    return dumptoggle;
}
