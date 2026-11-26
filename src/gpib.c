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
static int (*save_callback)(int);
static struct loader (*load_callback)(void);

/* LOAD transfer management */
volatile bool load_buffer_ready = false;
volatile uint8_t load_transfer_complete = 0;

/* LOAD drain state */
static volatile uint16_t load_drain_pos = 0;
volatile uint16_t load_half_count = 0;
volatile uint8_t load_last_half = 0;

void atn_isr(void) __interrupt(EXTI1_ISR)
{
    uint8_t atn = READ(ATN);
    if (atn && state == tx_paused) {
    gpib_set_talker_mode();
    } else {
        SET(NRFD);
        CLR(NDAC);
    }
    ++intcount;
}

void dav_isr(void) __interrupt(EXTI0_ISR)
{
    uint8_t atn = READ(ATN);
    uint8_t eoi = READ(EOI);
#if 0
    /* During talker mode (load), only handle UNT to end the transfer.
     * Do NOT touch NRFD/NDAC — the talker handshake manages those. */
    if (state == load) {
        if (READ(DAV) == 0 && !atn) {
            /* DAV low + ATN low = command byte from PET */
            uint8_t cmd = ~gpib_dataread();
            if (cmd == 0x5F) { /* UNT — PET ending talker phase */
                printf("DAV ISR: UNT detected (0x5F)\n");
                load_transfer_complete = 1;
                current_cmd = 0;   /* clear so CLOSE SCG goes to idle */
                state = idle;
            }
        }
        return; /* don't touch NRFD/NDAC during talker mode */
    }
#endif

    FLIP(BLUE_LED);
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
    /* Rising edge: PET deasserts NDAC (HIGH) = data accepted.
     * Deassert DAV, drain next byte if available.
     */
    if (load_buffer_ready) {
        CLR(DAV);
        /* Double-check volatile */
        if (load_buffer_ready) {
            gpib_load_drain();
        } else {
            CLR(DAV);
            load_buffer_ready = false;
            CCR2(NDAC);
        }
    }
}

void gpib_init()
{
    // read my device address from EEPROM
    myaddr = EEPROM;

    state = idle;

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

void gpib_set_talker_mode()
{
    // DAV: input -> output, drive low
    // Must drive DAV low explicitly: when switching from input (floating high)
    // to output, ODR state is unknown. PET may see a false DAV rising edge
    // and miss the data or read garbage.
    OUT(DAV);
    CLR(DAV);
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
    SCR2(NDAC);
    EXTI_CR1 = (EXTI_CR1 & ~0x30) | 0x10;  // PC2 rising edge only

    // Keep DAV interrupt enabled to detect UNT from PET during talker mode.
    // The ISR guards against interfering with the data handshake.

    // Disable EOI interrupt
    CCR2(EOI);
}

void gpib_set_listener_mode()
{
    // DAV: output -> input, enable interrupt
    IN(DAV);
    CCR1(DAV);
    SCR2(DAV);
    EXTI_CR1 = (EXTI_CR1 & ~0x03) | 0x03;  // rising or falling edge

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
    // Put byte on GPIB data bus
    gpib_datawrite(~byte);

    if (eoi) {
        SET(EOI);
    }

    // Deassert DAV — signals listener that new data is on bus
    CLR(DAV);

    // Wait for PET to be ready for data
    printf("TX: waiting NRFD... ");
    uint32_t nrfd_count = 0;
    uint32_t ndac_count = 0;
    while (READ(NRFD)) {
        nrfd_count++;
        if (nrfd_count > 100000) {
            printf("HANG NRFD=%u DAV=%u data=%02x\n",
                   READ(NRFD), READ(DAV), ~gpib_dataread() & 0xff);
            break;
        }
    }
    printf("NRFD ok\n");

    // Assert DAV — signal data valid
    SET(DAV);

    // Wait for PET to accept
    printf("TX: waiting NDAC... ");
    while (!READ(NDAC)) {
        ndac_count++;
        if (ndac_count > 100000) {
            printf("HANG NDAC=%u DAV=%u\n", READ(NDAC), READ(DAV));
            break;
        }
    }
    printf("NDAC ok\n");

    CLR(DAV); // data acknowledged

    // Wait for PET ready for next
    while (!READ(NDAC));

    if (eoi) {
        CLR(EOI);
    }
}

void gpib_load_start(void)
{
    load_buffer_ready = false;
    load_drain_pos = 0;
    load_half_count = 0;
    load_last_half = 0;
}

void gpib_load_drain(void)
{
#if 0
    uint16_t idx = load_drain_pos;
    uint8_t byte = sector_buf[idx];
    uint8_t eoi = 0;

    load_drain_pos++;

    /* Assert EOI on the very last byte of the file */
    if (load_drain_pos >= load_half_count && load_last_half) {
        eoi = 1;
    }

    printf("DRAIN: byte=%02x eoi=%d pos=%u/%u\n", byte, eoi, load_drain_pos, load_half_count);
    gpib_send_byte_talker(byte, eoi);
    printf("DRAIN: done\n");

    /* If this half is fully drained, signal main loop to read more */
    if (load_drain_pos >= load_half_count) {
        load_buffer_ready = false;
        printf("DRAIN: half done, buffer_ready=false\n");
    }
#endif
}

uint8_t gpib_load_active(void)
{
    return load_buffer_ready;
}

void gpib_load_complete(void)
{
    load_buffer_ready = false;
    disableInterrupts();
    CCR2(NDAC);
    CLR(DAV);
    enableInterrupts();
}

uint8_t gpib_eoi() { return READ(EOI); }
uint8_t gpib_atn() { return READ(ATN); }
uint8_t gpib_dav() { return READ(DAV); }
uint8_t gpib_nrfd() { return READ(NRFD); }
uint8_t gpib_ndac() { return READ(NDAC); }
uint8_t gpib_address() { return myaddr; }

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

void gpib_set_load_callback(struct loader (*cb)(void))
{
    load_callback = cb;
}
