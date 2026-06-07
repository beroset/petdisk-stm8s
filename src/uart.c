#include "stm8.h"
#include "pindefs.h"
// definitions for SR
#define UART_TXE    7
#define UART_TC     6
#define UART_RXNE   5
// definitions for CR2
#define UART_TEN    3
#define UART_REN    2

#define UART_BIT_RATE    9600
#define UART_BIT_RATE_DIVISOR   (MASTER_FREQ/UART_BIT_RATE)

/*
 * PD5 -> TX
 * PD6 -> RX
 */
void uart_init() {
    UART2_BRR2 = (UART_BIT_RATE_DIVISOR & 0x0f) | (UART_BIT_RATE_DIVISOR >> 12);
    UART2_BRR1 = (UART_BIT_RATE_DIVISOR >> 4);
    UART2_CR2 = (1 << UART_TEN) | (1 << UART_REN);
}

void uart_write(uint8_t data) {
    UART2_DR = data;
    while (!(UART2_SR & (1 << UART_TC)));
}

uint8_t uart_read() {
    while (!(UART2_SR & (1 << UART_RXNE)));
    return UART2_DR;
}

int putchar(int c) {
    uart_write(c);
    return 0;
}

int getchar() {
    return uart_read();
}
