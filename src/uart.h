#ifndef UART_H
#define UART_H
#include <stdint.h>
void uart_init();
void uart_write(uint8_t data);
uint8_t uart_read();
int kbhit();
#endif // UART_H
