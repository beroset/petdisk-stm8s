#ifndef PINDEFS_H
#define PINDEFS_H

/* pin definitions

    Display | STM8
    --------|-------
    RS      | PD0
    RW      | PC1
    EN      | PD2
    D4      | PB0
    D5      | PB1
    D6      | PB2
    D7      | PB3

*/

#define RS_PORT D
#define RS_BIT 0
#define RW_PORT C
#define RW_BIT 1
#define EN_PORT D
#define EN_BIT 2

#define LED_PORT C
#define LED_BIT 5
#define CLOCK_PORT D
#define CLOCK_BIT 7

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

#endif // PINDEFS_H


