#include "spi.h"
#include "stm8.h"
#include "pindefs.h"
// SPI CR1
#define SPE         6
#define BR2         5
#define BR1         4
#define BR0         3
#define MSTR        2
#define CPOL        1
#define CPHA        0
// SPI CR2
#define BDM         7 // 0=2-line unidirectional mode
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
    SPI_CR1 = (1u << SPE) | (1u << BR1) | (1u << MSTR);
    SPI_CR2 = (1u << SSM) | (1u << SSI);
    (void)SPI_DR;
    (void)SPI_SR;
}

static uint8_t SPI_transfer(uint8_t data)
{
    SPI_DR = data;
    while (!(SPI_SR & (1 << TXE)));
    while (!(SPI_SR & (1 << RXNE)));
    return SPI_DR;
}

void SPI_write(uint8_t data)
{
    (void)SPI_transfer(data);
}

uint8_t SPI_read()
{
    return SPI_transfer(0xFF);
}

void chip_select() {
    CLR(CS);
}

void chip_deselect() {
    const uint16_t spi_busy_timeout = 0xFFFF;
    uint16_t timeout = spi_busy_timeout;

    while ((SPI_SR & (1 << BSY)) && --timeout);
    if (!timeout) {
        SPI_CR1 &= (uint8_t)~(1u << SPE);
        SPI_CR1 |= (1u << SPE);
    }
    SET(CS);
}
