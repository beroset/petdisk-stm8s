#include "spi.h"
#include "stm8.h"

/*
 * I have a SPI module that is 5V compatible and has both a 
 * level converter and a 5V to 3.3V converter.  These are
 * the connections to the ST8S development board.
 *
 * Module  |  Devboard  | CPU
 * --------|------------|--------
 *   CS    |  D10       | PE5
 *   SCL   |  D13       | PC5
 *   MOSI  |  D11       | PC6
 *   MISO  |  D12       | PC7
 *   VCC   |  +5V       | +5V
 *   GND   |  GND       | GND
 *
 * Note that the CS pin assignment can be changed, but the 
 * three other SPI pins are fixed.
 */

#define CS_PORT E
#define CS_BIT 5
#define SCL_PORT C
#define SCL_BIT 5
#define MOSI_PORT C
#define MOSI_BIT 6
#define MISO_PORT C
#define MISO_BIT 7

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

