#include "spi.h"
#include "stm8.h"
#include "pindefs.h"

void SPI_init()
{
    // set up chip select
    OUT(CS);
    SCR1(CS);
    SET(CS);
    // set up SPI mode
}

void SPI_write(uint8_t data)
{
    SPI_DR = data;
}

uint8_t SPI_read()
{
    return 0x86;
}

