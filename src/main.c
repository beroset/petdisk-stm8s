#include <stdint.h>
#include <stdio.h>
#include "stm8.h"
#include "timer.h"
#include "uart.h"
#include "pindefs.h"
#include "ff.h"
#include "diskio.h"
#include "gpib.h"

void timer2_isr(void) __interrupt(TIM2_OVF_ISR)
{
    // clear IT pending bit
    BITCLR(TIM2_SR1, 0);
    // toggle LED
    FLIP(LED);
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

    timer_init();

    // set up LED output
    OUT(LED);
    SCR1(LED);
    CCR2(LED);

    uart_init();
    gpib_init();
}

static void reset_on_halt()
{
    WWDG_CR = 0x80;
}

static void dumpmem(const BYTE* buff, UINT sz) {
    for (UINT n = 0; n < sz; ++n, ++buff) {
        if ((n & 0xf) == 0) {
            printf("\n%04x:", n);
        }
        printf(" %02x", *buff & 0xff);
    }
    putchar('\n');
}

#define BUFLEN 512
#define HALF BUFLEN/2
volatile uint8_t sector_buf[BUFLEN];
static volatile uint16_t sector_buf_idx = 0;
static volatile bool A_full = false;
static volatile bool B_full = false;
static bool talking = false;
static bool awaiting_ack = false;

static FIL file;			/* File object */

static int save_callback(int cmd)
{
    if (sector_buf_idx < BUFLEN) {
        sector_buf[sector_buf_idx++] = cmd;
        if (sector_buf_idx == HALF) {
            A_full = true;
        }
        if (sector_buf_idx == BUFLEN) {
            B_full = true;
            sector_buf_idx = 0;  // wrap around
        }
    }
    return 0;
}

void main(void)
{
    init();
    gpib_set_save_callback(save_callback);
    enableInterrupts();
    puts(" Hello\nPETski!");
    FRESULT rc;
    // disk things
    FATFS fatfs;			// File system object
    FSIZE_t filesize = 0;               // file size (for LOAD)
    enum file_state { idle, writing, reading } fstate = idle;

    rc = f_mount(&fatfs, "", 0);
    for (;;) {
        enum bus_state state = gpib_state();
        switch (fstate) {
            case idle:
                switch (state) {
                    case load:
                        printf("Opening %s for load, ", gpib_filename());
                        rc = f_open(&file, gpib_filename(), FA_READ);
                        // try again if it doesn't work the first time
                        if (rc == FR_NOT_READY) {
                            rc = f_open(&file, gpib_filename(), FA_READ);
                        }
                        printf("rc = 0x%02x\n", rc);
                        if (rc == FR_OK) {
                            filesize = f_size(&file);
                            printf("filesize = %lu\n", filesize);
                        }
                        talking = true;
                        awaiting_ack = false;
                        A_full = true;
                        sector_buf_idx = 0;
                        fstate = reading;
                        break;
                    case save:
                        printf("Opening %s for save, ", gpib_filename());
                        rc = f_open(&file, gpib_filename(), FA_WRITE | FA_CREATE_ALWAYS);
                        // try again if it doesn't work the first time
                        if (rc == FR_NOT_READY) {
                            rc = f_open(&file, gpib_filename(), FA_WRITE | FA_CREATE_ALWAYS);
                        }
                        printf("rc = 0x%02x\n", rc);
                        sector_buf_idx = 0;
                        fstate = writing;
                        break;
                }
                break;
            case writing:
                if (A_full) {
                    UINT bw;
                    f_write(&file, sector_buf, HALF, &bw);
                    f_sync(&file);
                    puts("Writing A_buff");
                    A_full = false;
                }
                if (B_full) {
                    UINT bw;
                    f_write(&file, sector_buf+HALF, HALF, &bw);
                    f_sync(&file);
                    puts("Writing B_buff");
                    B_full = false;
                }
                if (state == idle) {
                    // write any remaining bytes
                    UINT bw;
                    bool A = sector_buf_idx < HALF;
                    f_write(&file, sector_buf + (A ? 0 : HALF), sector_buf_idx - (A ? 0 : HALF), &bw);
                    printf("Writing from %c, %d (0x%0x) bytes\n",
                        (sector_buf_idx < HALF ? 'A' : 'B'),
                        sector_buf_idx - (A ? 0 : HALF),
                        sector_buf_idx - (A ? 0 : HALF)
                    );
                    f_sync(&file);
                    puts("Closing file");
                    f_close(&file);
                    sector_buf_idx = 0;
                    fstate = idle;
                }
                break;
            case reading:
                /*
                 * We are reading from disk and sending each byte 
                 * to the PET over GPIB.  We send a single byte at
                 * a time from here, and the NDAC-driven interrupt
                 * signals when it's time to send the next one.
                 *
                 * This continues until the final byte which is sent
                 * with EOI true.
                 */
                if (!talking && filesize == 0) {
                    // PET has read the last byte
                    puts("Last byte acked: Closing file");
                    f_close(&file);
                    sector_buf_idx = 0;
                    fstate = idle;
                    gpib_set_listener_mode();
                } else {
                    // If buffer not ready, read next chunk from SD card
                    if (A_full) {
                        UINT br;
                        rc = f_read(&file, sector_buf, BUFLEN, &br);
                        if (rc != FR_OK) {
                            printf("Only read %d bytes, error: %d\n", br, rc);
                            // artificially send 0 byte with EOI set
                            awaiting_ack = true;
                            gpib_send_byte_talker(0, true);
                            filesize = 0;
                        } else {
                            printf("Read %d bytes\n", br);
                        }
                        A_full = false;
                    }
                    if (!awaiting_ack || gpib_acked()) {
                        --filesize;
                        awaiting_ack = true;
                        gpib_send_byte_talker(sector_buf[sector_buf_idx], filesize == 0);
                        if (filesize == 0) {
                            talking = false;
                        } else {
                            // more to go
                            ++sector_buf_idx;
                            // wrap index and ask for next chunk
                            if (sector_buf_idx == BUFLEN) {
                                sector_buf_idx = 0;
                                A_full = true;
                            }
                        }
                    }
                }
            }
        if (kbhit()) {
            int command = getchar();
            switch (command) {
                case 'a':  // read GPIB data bus
                    printf("Device address: %d\n", gpib_address());
                    break;
                case 'b':  // read GPIB data bus
                    gpib_dump();
                    break;
                case 'c':  // read EXTI_CR1 and CR2
                    printf("EXTI_CR1: %02x, EXTI_CR2: %02x\n", EXTI_CR1, EXTI_CR2);
                    break;
                case 'd':  // poke data onto bus
                    printf("GPIB data is %s\n", 
                        gpib_data_dump(0x55) ?
                        "INPUT" :
                        "OUTPUT"
                    );
                    break;
                case 'f':  // dump buffer
                    dumpmem(sector_buf, BUFLEN);
                    break;
                case 'i':  // read GPIB int count
                    printf("intcount: %lu\n", gpib_intcount());
                    break;
                case 'l':  // toggle blue LED
                    FLIP(BLUE_LED);
                    break;
                case 'm':  // dump memory for filename
                    dumpmem(gpib_filename(), 32);
                    break;
                case 'r':
                    puts("Resetting now...\n");
                    f_mount(0, "", 1);
                    delay_ms(1000);
                    reset_on_halt();
                    break;
                case 's':
                    printf("Serial number");
                    dumpmem((BYTE *)0x48cd, 12);
                    break;
                default:
                    // do nothing
            }
        }
        waitForInterrupt();
    }
}
