#include <stdint.h>
#include <stdio.h>
#include "stm8.h"
#include "timer.h"
#include "uart.h"
#include "pindefs.h"
#include "diskio.h"
#include "pff.h"

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
    printf("flag %x\n", fat->flag);
    printf("csize %d\n", fat->csize);
    printf("fatbase %ld\n", fat->fatbase);
    printf("dirbase %ld\n", fat->dirbase);
}

void main(void)
{
    init();
    enableInterrupts();
    puts(" Hello\nPETski!");
    FRESULT rc;

    // disk things
    FATFS fatfs;			/* File system object */
    DIR dir;				/* Directory object */
    FILINFO fno;			/* File information object */
    UINT bw, br, i;
    BYTE buff[64];

    printf("\nMount a volume.\n");
    rc = pf_mount(&fatfs);
    if (rc) die(rc);
    fatprint(&fatfs);

    if (!rc) {
        printf("\nOpen a test file (message.txt).\n");
        rc = pf_open("MESSAGE.TXT");
        if (rc) die(rc);
    }
    if (!rc) {
        printf("\nType the file content.\n");
        for (;;) {
                rc = pf_read(buff, sizeof(buff), &br);	/* Read a chunk of file */
                if (rc || !br) break;			/* Error or end of file */
                for (i = 0; i < br; i++)		/* Type the data */
                        putchar(buff[i]);
        }
        if (rc) die(rc);
    }

#if PF_USE_WRITE
    if (!rc) {
        printf("\nOpen a file to write (write.txt).\n");
        rc = pf_open("WRITE.TXT");
        if (rc) die(rc);
    }

    if (!rc) {
        printf("\nWrite a text data. (Hello world!)\n");
        for (;;) {
                rc = pf_write("Hello world!\r\n", 14, &bw);
                if (rc || !bw) break;
        }
        if (rc) die(rc);
    }
    if (!rc) {
        printf("\nTerminate the file write process.\n");
        rc = pf_write(0, 0, &bw);
        if (rc) die(rc);
    }
#endif

#if PF_USE_DIR
    if (!rc) {
        printf("\nOpen root directory.\n");
        rc = pf_opendir(&dir, "");
        if (rc) die(rc);
    }
    if (!rc) {
        printf("\nDirectory listing...\n");
        for (;;) {
                rc = pf_readdir(&dir, &fno);	/* Read a directory item */
                if (rc || !fno.fname[0]) break;	/* Error or end of dir */
                if (fno.fattrib & AM_DIR)
                        printf("   <dir>  %s\n", fno.fname);
                else
                        printf("%8lu  %s\n", fno.fsize, fno.fname);
        }
        if (rc) die(rc);
    }
#endif

    if (rc) {
        printf("\nTest FAILED.\n");
    } else {
        printf("\nTest completed.\n");
    }


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
