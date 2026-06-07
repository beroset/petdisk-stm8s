#ifndef PINDEFS_H
#define PINDEFS_H

#define MASTER_FREQ 2000000

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

#define RS_PORT C
#define RS_BIT 1
#define RW_PORT C
#define RW_BIT 2
#define EN_PORT C
#define EN_BIT 3
#define DISPLAY_SPAN_PORT B
#define DISPLAY_SPAN_MASK 0x0f
#define DISPLAY_SPAN_SHIFT 0

/*
 * PB0 to PB3 are LCD DB4-DB7
 */

/*
 * SPI module
 *
 * Module  |  CPU  | color
 * --------|-------|------
 *   CS    |  PC4  | BLU
 *   SCL   |  PC5  | GRN
 *   MOSI  |  PC6  | YEL
 *   MISO  |  PC7  | ORG
 *   VCC   |  VDD  | RED
 *   GND   |  VSS  | BRN
 *
 * Note that the CS pin assignment can be changed, but the
 * three other SPI pins are fixed.
 */

#define CS_PORT C
#define CS_BIT 4
#define SCL_PORT C
#define SCL_BIT 5
#define MOSI_PORT C
#define MOSI_BIT 6
#define MISO_PORT C
#define MISO_BIT 7

#ifdef DEVBOARD
//#warning "LED is defined as PD7, not on-board LED"
/*
 * Note that because the on-board LED is on C5 which is also the SCL,
 * we can't use it.
 */
/*
 * LED
 */
#define LED_PORT D
#define LED_BIT 7
#else
//#warning "DEVBOARD is NOT defined"

/*
 * LED
 */
#define LED_PORT E
#define LED_BIT 5

/*
 * IEEE-488 bus
 */
/*
 *  IEEE-488 |  CPU  | color
 * ----------|-------|------
 *   DAV     |  PA1  | BRN
 *   NRFD    |  PA2  | RED
 *   NDAC    |  PF4  | ORG
 *   ATN     |  PB5  | YEL
 *   EOI     |  PB4  | WHT
 *
 * GND - BLK
 *   Unused: SRQ, IFC, REN
 */

#define DAV_PORT  A
#define DAV_BIT   1
#define NRFD_PORT A
#define NRFD_BI   2
#define NDAC_PORT F
#define NDAC_BIT  4
#define ATN_PORT  B
#define ATN_BIT   5
#define EOI_PORT  B
#define EOI_BIT   4

// PD0-PD7 => DIO1-DIO8
#define GPIB_SPAN_PORT D
#define GPIB_SPAN_MASK 0xff
#define GPIB_SPAN_SHIFT 0

#endif // DEVBOARD
#endif // PINDEFS_H


