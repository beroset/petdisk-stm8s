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

void show_gpib()
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
           gpib_dataread() & 0xff,
           statenames[gpib_state()]);
}

#define BUFLEN 512
#define HALF BUFLEN/2
volatile uint8_t sector_buf[BUFLEN];
static volatile uint16_t sector_buf_idx = 0;
static volatile bool A_end = false;
static volatile bool B_end = false;
static bool eof = false;
static uint16_t eof_index = 0;

static FIL file;			/* File object */

static int save_callback(int cmd)
{
    if (sector_buf_idx < BUFLEN) {
        sector_buf[sector_buf_idx++] = cmd;
        if (sector_buf_idx == HALF) {
            A_end = true;
        }
        if (sector_buf_idx == BUFLEN) {
            B_end = true;
            sector_buf_idx = 0;  // wrap around
        }
    }
    return 0;
}

static struct loader load_callback(void)
{
    static struct loader l;
    l.byte = sector_buf[sector_buf_idx];
    l.eoi = (eof && sector_buf_idx == eof_index);
    ++sector_buf_idx;
    if (sector_buf_idx == BUFLEN) {
        sector_buf_idx = 0;
    }
    return l;
}

void main(void)
{
    init();
    gpib_set_save_callback(save_callback);
    gpib_set_load_callback(load_callback);
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
                        A_end = B_end = true;
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
                        fstate = writing;
                        break;
                }
                break;
            case writing:
                if (A_end) {
                    UINT bw;
                    f_write(&file, sector_buf, HALF, &bw);
                    f_sync(&file);
                    puts("Writing A_buff");
                    A_end = false;
                }
                if (B_end) {
                    UINT bw;
                    f_write(&file, sector_buf+HALF, HALF, &bw);
                    f_sync(&file);
                    puts("Writing B_buff");
                    B_end = false;
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
                if (A_end) {
                    UINT br;
                    f_read(&file, sector_buf, HALF, &bw);
                }
                /* Interrupt-driven LOAD transfer.
                 * Main loop: reads HALF-byte chunks from SD card
                 * ISR (NDAC): drains one byte per PET NDAC pulse
                 */
                {
                    UINT br;

                    // Switch to talker mode on first entry
                    static bool talking = false;
                    if (!talking) {
                        printf("SETUP: entering talker mode\n");
                        gpib_load_start();
                        gpib_set_talker_mode();
                        printf("SETUP: talker mode ready\n");
                        talking = true;
                    }

                    // If buffer not ready, read next chunk from SD card
                    if (!load_buffer_ready) {
                        // If UNT was detected by the DAV ISR, clean up immediately
                        if (load_transfer_complete) {
                            gpib_load_complete();
                            gpib_set_listener_mode();
                            f_close(&file);
                            talking = false;
                            fstate = idle;
                            puts("UNT detected, transfer stopped");
                            break;
                        }

                        rc = f_read(&file, sector_buf, HALF, &br);
                        // Verify first few bytes of data after successful read
                        if (br > 0 && talking) {
                            printf("SETUP: first bytes: %02x %02x %02x\n",
                                   sector_buf[0], sector_buf[1], sector_buf[2]);
                            talking = false; // only print once
                        }
                        if (rc != FR_OK) {
                            // Error — wait for any pending drain, then send EOF
                            while (load_buffer_ready && !load_transfer_complete) {
                                waitForInterrupt();
                            }
                            // If UNT arrived during wait, skip EOF
                            if (load_transfer_complete) {
                                gpib_load_complete();
                                gpib_set_listener_mode();
                                f_close(&file);
                                talking = false;
                                fstate = idle;
                                puts("UNT during error");
                                break;
                            }
                            gpib_send_byte_talker(0x00, 1);
                            gpib_load_complete();
                            gpib_set_listener_mode();
                            f_close(&file);
                            talking = false;
                            fstate = idle;
                            printf("Error reading file: %d\n", rc);
                            break;
                        }
                        if (br == 0) {
                            // Normal EOF — wait for pending bytes to drain,
                            // or UNT from PET, then clean up
                            while (load_buffer_ready && !load_transfer_complete) {
                                waitForInterrupt();
                            }
                            if (load_transfer_complete) {
                                gpib_load_complete();
                                gpib_set_listener_mode();
                                f_close(&file);
                                talking = false;
                                fstate = idle;
                                puts("UNT at EOF");
                                break;
                            }
                            gpib_load_complete();
                            gpib_set_listener_mode();
                            f_close(&file);
                            talking = false;
                            fstate = idle;
                            puts("br = 0");
                            break;
                        } else {
                            printf("Read %d (0x%02x) bytes\n", br, br);
                        }
                        // Set drain state for ISR
                        load_half_count = br;
                        load_last_half = (f_tell(&file) >= filesize) ? 1 : 0;
                        load_buffer_ready = true;

                        // Manually trigger first byte to kick off the handshake.
                        // The PET is holding NDAC low (waiting for STM8 to initiate),
                        // so the NDAC ISR will never fire on its own.
                        gpib_load_drain();
                    }
                }
                break;
        }
        if (kbhit()) {
            int command = getchar();
            switch (command) {
                case 'a':  // read GPIB data bus
                    printf("Device address: %d\n", gpib_address());
                    break;
                case 'b':  // read GPIB data bus
                    show_gpib();
                    break;
                case 'c':  // read EXTI_CR1 and CR2
                    printf("EXTI_CR1: %02x, EXTI_CR2: %02x\n", EXTI_CR1, EXTI_CR2);
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
