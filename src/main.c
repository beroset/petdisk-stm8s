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

void die (		/* Stop with dying message */
	FRESULT rc	/* FatFs return value */
)
{
	printf("Failed with rc=%u.\n", rc);
}
void fatprint(FATFS *fat)
{
    printf("fs_type %d\n", fat->fs_type);
    printf("csize %d\n", fat->csize);
    printf("fatbase %ld\n", fat->fatbase);
    printf("dirbase %ld\n", fat->dirbase);
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

static void dumpdir(const DIR* dir) {
    printf("size of dir = %d\n", sizeof(DIR));
    printf("dptr = 0x%lx, clust = 0x%lx, sect = 0x%lx\n\"", dir->dptr, dir->clust, dir->sect);
    for (int i=0; i < 11; ++i) {
        if (i == 8) putchar('.');
        putchar(dir->fn[i]);
    }
    dumpmem(dir->dir, 16);
    printf("\" flag = %02x\n", dir->fn[11]);
}

void test() {
    FRESULT rc;
    // disk things
    FATFS fatfs;			/* File system object */
    FIL file;			/* File object */
    UINT bw, br, i;
    uint8_t buff[64];
    static const char write_text[] = "Hello world!\r\n";

    printf("\nMount a volume.\n");
    rc = f_mount(&fatfs, "", 0);
    if (rc) die(rc);

    if (!rc) {
        printf("\nOpen a test file (message.txt).\n");
        rc = f_open(&file, "MESSAGE.TXT", FA_OPEN_ALWAYS | FA_READ);
        // try again if it doesn't work the first time
        if (rc == FR_NOT_READY) {
            rc = f_open(&file, "MESSAGE.TXT", FA_OPEN_ALWAYS | FA_READ);
        }
        if (rc) die(rc);
    }
    fatprint(&fatfs);
    if (!rc) {
        printf("\nType the file contents.\n");
        for (;;) {
            rc = f_read(&file, &buff, sizeof(buff), &br);	/* Read a chunk of file */
            if (rc || !br) break;			/* Error or end of file */
            for (i = 0; i < br; i++)		/* Type the data */
                putchar(buff[i]);
        }
        if (rc) die(rc);
    }
    rc = f_close(&file);
    if (rc) die(rc);

    if (!rc) {
        printf("\nOpen a file to write (write.txt).\n");
        rc = f_open(&file, "WRITE.TXT", FA_WRITE | FA_READ | FA_CREATE_ALWAYS);
        if (rc) die(rc);
    }
    if (!rc) {
        printf("\nWrite a text data. (Hello world!)\n");
        rc = f_write(&file, write_text, sizeof(write_text) - 1, &bw);
        if (!rc && bw != sizeof(write_text) - 1) rc = FR_DISK_ERR;
        if (rc) die(rc);
    }
    if (!rc) {
        printf("\nClose written file.\n");
        rc = f_close(&file);
        if (rc) die(rc);
    }
    if (rc) {
        printf("\nTest FAILED.\n");
    } else {
        printf("\nTest completed.\n");
    }
    f_mount(0, "", 1);
}

void show_gpib()
{
    printf("EOI ATN DAV NRFD NDAC  data\n"
           " %u   %u   %u    %u    %u    %02x\n",
           gpib_eoi() != 0,
           gpib_atn() != 0,
           gpib_dav() != 0,
           gpib_nrfd() != 0,
           gpib_ndac() != 0,
           gpib_data() & 0xff);
}

#define BUFLEN 512
#define HALF BUFLEN/2
static uint8_t sector_buf[BUFLEN];
static volatile uint16_t sector_buf_idx = 0;
static volatile bool A_full = false;
static volatile bool B_full = false;

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
    gpib_setcallback(save_callback);
    enableInterrupts();
    puts(" Hello\nPETski!");
    FRESULT rc;
    // disk things
    FATFS fatfs;			/* File system object */
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
                case 't':
                    f_mount(0, "", 1);
                    test();
                    break;
                default:
                    // do nothing
            }
        }
        waitForInterrupt();
    }
}
