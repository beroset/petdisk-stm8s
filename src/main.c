#include <stdint.h>
#include <stdio.h>
#include "stm8.h"
#include "display.h"
#include "timer1ms.h"

#define LEDBIT 5
#define CLOCKBIT 7

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
    BITFLIP(PC_ODR, LEDBIT);
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

    timer1ms_init();

    // set up LED output
    PC_DDR = 1u << LEDBIT;
    PC_CR1 = 1u << LEDBIT;
    PC_CR2 = 0;

    // set PD7 also as output
    PD_DDR = 1u << CLOCKBIT;
    PD_CR1 = 1u << CLOCKBIT;
    PD_ODR = 1u << CLOCKBIT;
}

static const char* hex = "0123456789ABCDEF";

void main(void)
{
    init();
    enableInterrupts();
    display_reset();
    display_print(" Hello\nPETski!");
    delay_ms(2000);
    uint8_t addr = display_readaddr();
    display_clear();
    char msg[] = "00 ";
    msg[0] = hex[(addr >> 4) & 0x0f];
    msg[1] = hex[addr & 0x0f];
    display_print(msg);

    for (;;) {
        delay_ms(3);  // wait 3ms
        BITFLIP(PD_ODR, CLOCKBIT); // toggle pin
        waitForInterrupt();
    }
}
