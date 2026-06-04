#include "spi.h"
#include "stm8.h"
#include "pindefs.h"
// SPI CR1
#define SPE         6
#define BR0         3
#define MSTR        2
// SPI CR2
#define SSM         1
#define SSI         0
// SPI SR
#define BSY         7
#define TXE         1
#define RXNE        0

/* 
 * I found this web site useful and have based much of this code on it:
 * https://lujji.github.io/blog/bare-metal-programming-stm8/
 */

void SPI_init()
{
    // set up chip select
    OUT(CS);
    SCR1(CS);
    SET(CS);
    // set up SPI mode
    SPI_CR2 = (1u << SSM) | (1u << SSI);
    SPI_CR1 = (1u << SPE) | (1u << BR0) | (1u << MSTR);
}

void SPI_write(uint8_t data)
{
    SPI_DR = data;
    while (!(SPI_SR & (1 << TXE)));
}

uint8_t SPI_read()
{
    SPI_write(0xFF);
    while (!(SPI_SR & (1 << RXNE)));
    return SPI_DR;
}

void chip_select() {
    CLR(CS);
}

void chip_deselect() {
    while ((SPI_SR & (1 << BSY)));
    SET(CS);
}
