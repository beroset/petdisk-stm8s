#include <stdint.h>

#define CLK_DIVR	(*(volatile uint8_t *)0x50c6)
#define CLK_PCKENR1	(*(volatile uint8_t *)0x50c7)

#define TIM1_CR1	(*(volatile uint8_t *)0x5250)
#define TIM1_IER	(*(volatile uint8_t *)0x5254)
#define TIM1_SR1	(*(volatile uint8_t *)0x5255)
#define TIM1_CNTRH	(*(volatile uint8_t *)0x525e)
#define TIM1_CNTRL	(*(volatile uint8_t *)0x525f)
#define TIM1_PSCRH	(*(volatile uint8_t *)0x5260)
#define TIM1_PSCRL	(*(volatile uint8_t *)0x5261)
#define TIM1_ARRH	(*(volatile uint8_t *)0x5262)
#define TIM1_ARRL	(*(volatile uint8_t *)0x5263)

#define TIM2_CR1	(*(volatile uint8_t *)0x5300)
#define TIM2_IER	(*(volatile uint8_t *)0x5301)
#define TIM2_SR1	(*(volatile uint8_t *)0x5302)
#define TIM2_SR2	(*(volatile uint8_t *)0x5303)
#define TIM2_EGR	(*(volatile uint8_t *)0x5304)
#define TIM2_CNTRH	(*(volatile uint8_t *)0x530a)
#define TIM2_CNTRL	(*(volatile uint8_t *)0x530b)
#define TIM2_PSCR	(*(volatile uint8_t *)0x530c)
#define TIM2_ARRH	(*(volatile uint8_t *)0x530d)
#define TIM2_ARRL	(*(volatile uint8_t *)0x530e)


#define PC_ODR	(*(volatile uint8_t *)0x500a)
#define PC_IDR	(*(volatile uint8_t *)0x500b)
#define PC_DDR	(*(volatile uint8_t *)0x500c)
#define PC_CR1	(*(volatile uint8_t *)0x500d)
#define PC_CR2	(*(volatile uint8_t *)0x500e)

const uint8_t LED_PIN = (1 << 5);

#if 0
unsigned char __sdcc_external_startup(void) {
    return 0;
}
#endif 

#define USING_TIMER_INTERRUPT 0
#if USING_TIMER_INTERRUPT
void timer2_isr(void) __interrupt(13) 
{
    PC_ODR ^= LED_PIN;
    // clear IT pending bit
    TIM2_SR1 &= ~0x01; // clear interrupt
}
#endif

#define enableInterrupts()    {__asm__("rim\n");}  /* enable interrupts */
#define disableInterrupts()   {__asm__("sim\n");}  /* disable interrupts */

const uint16_t reload_value = 15625;

unsigned int clock(void)
{
    unsigned char h = TIM1_CNTRH;
    unsigned char l = TIM1_CNTRL;
    return((unsigned int)(h) << 8 | l);
}

void main(void)
{
    CLK_DIVR = 0x18; // Set the frequency to 2 MHz
#if USING_TIMER_INTERRUPT
    enableInterrupts();
    TIM2_PSCR = 0b00000111;  // prescaler = 128
    TIM2_ARRH = reload_value >> 8;
    TIM2_ARRL = reload_value & 0x00ff;
    TIM2_IER = 0x01;  // update interrupt enable
    TIM2_CR1 = 0x01;
#else
    // Configure timer
    // 1000 ticks per second
    TIM1_PSCRH = 0x07;
    TIM1_PSCRL = 0xd0;
#endif

    PC_DDR = LED_PIN;
    PC_CR1 = LED_PIN;
    PC_CR2 = ~LED_PIN;
    // Enable timer
#if USING_TIMER_INTERRUPT

    for(;;) {
        // do nothing at all!
    }
#else
    // Enable timer
    TIM1_CR1 = 0x81;
    for(;;) {
        if (clock() % 1000 < 200) {
            PC_ODR ^= LED_PIN;
            for (int i=5000; i; --i)
                __asm__ ("nop\n");
        } 
    }
#endif
}
