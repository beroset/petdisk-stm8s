#include <stdint.h>
#include <stdio.h>
#include "stm8.h"
#include "timer.h"
#include "uart.h"
#include "pindefs.h"
#include "ff.h"
#include "diskio.h"

#if 0
unsigned char __sdcc_external_startup(void) {
    return 0;
}
#endif

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

FRESULT list_dir (const char *path)
{
    FRESULT res;
    DIR dir;
    FILINFO fno;
    int nfile, ndir;


    res = f_opendir(&dir, path);                   /* Open the directory */
    if (res == FR_OK) {
        nfile = ndir = 0;
        for (;;) {
            res = f_readdir(&dir, &fno);           /* Read a directory item */
            if (fno.fname[0] == 0) break;          /* Error or end of dir */
            if (fno.fattrib & AM_DIR) {            /* It is a directory */
                printf("   <DIR>   %s\n", fno.fname);
                ndir++;
            } else {                               /* It is a file */
                printf("%10lu %s\n", fno.fsize, fno.fname);
                nfile++;
            }
        }
        f_closedir(&dir);
        printf("%d dirs, %d files.\n", ndir, nfile);
    } else {
        printf("Failed to open \"%s\". (%u)\n", path, res);
    }
    return res;
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

    if (!rc) {
        printf("\nRead root directory.\n");
        rc = list_dir("");
        if (rc) die(rc);
    }
    if (rc) {
        printf("\nTest FAILED.\n");
    } else {
        printf("\nTest completed.\n");
    }
    f_mount(0, "", 1);
}

void main(void)
{
    init();
    enableInterrupts();
    puts(" Hello\nPETski!");
    test();

    for (;;) {
        int command = getchar();
        switch (command) {
            case 'r':
                puts("Resetting now...\n");
                delay_ms(1000);
                reset_on_halt();
                break;
            default:
                // do nothing
        }
        waitForInterrupt();
    }
}
