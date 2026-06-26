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
#define SPI_BUSY_TIMEOUT 0xFFFF
#define SPI_TRANSFER_TIMEOUT 0xFFFF

/*
 * I found this web site useful and have based much of this code on it:
 * https://lujji.github.io/blog/bare-metal-programming-stm8/
 */

static void SPI_recover(void)
{
    SPI_CR1 &= (uint8_t)~(1u << SPE);
    SPI_CR1 |= (1u << SPE);
}

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
    uint16_t timeout = SPI_TRANSFER_TIMEOUT;

    SPI_DR = data;
    while (!(SPI_SR & (1 << TXE)) && timeout > 0) {
        timeout--;
    }
    if (!timeout) {
        SPI_recover();
        return 0xFF;
    }

    timeout = SPI_TRANSFER_TIMEOUT;
    while (!(SPI_SR & (1 << RXNE)) && timeout > 0) {
        timeout--;
    }
    if (!timeout) {
        SPI_recover();
        return 0xFF;
    }

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
    uint16_t timeout = SPI_BUSY_TIMEOUT;

    while ((SPI_SR & (1 << BSY)) && timeout > 0) {
        timeout--;
    }
    if (!timeout) {
        // If BSY never clears, reset SPI to recover from a stuck bus state.
        SPI_recover();
    }
    SET(CS);
}
