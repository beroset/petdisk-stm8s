// display.c
#include "display.h"
#include "timer.h"
#include "stm8.h"
#include "pindefs.h"

static void write_half(uint8_t value)
{
    CLR(RW);
    // write high bits to data lines
    WRITE_SPAN(DISPLAY, value);
    SET(EN);
    __asm__("nop"); // delay_us(1);
    CLR(EN);
    delay_us(50);
}

static uint8_t read_half()
{
    uint8_t data;
    SET(EN);
    __asm__("nop"); // delay_us(1);
    data = READ_SPAN(DISPLAY);
    CLR(EN);
    // delay_us(1);
    return data;
}

uint8_t display_readaddr()
{
    CLR(RS); // read register
    SET(RW); // change to read mode
    IN_SPAN(DISPLAY); // set direction to input data
    // read status nybbles
    uint8_t valhi = read_half() << 4;
    uint8_t vallo = read_half();
    OUT_SPAN(DISPLAY); // change back to output data
    CLR(RW); // and write
    return valhi | vallo;
}

static void write(uint8_t value)
{
    write_half(value >> 4);
    // write low bits to data lines
    write_half(value);
}

/*
 * RS = low
 */
static void write_reg(uint8_t value)
{
    CLR(RS);
    write(value);
    for (uint8_t addr = display_readaddr(); addr & 0x80; ) {
        addr = display_readaddr();
    }
}

/*
 * RS = high
 */
static void write_data(uint8_t value)
{
    SET(RS);
    write(value);
    for (uint8_t addr = display_readaddr(); addr & 0x80; ) {
        addr = display_readaddr();
    }
}

void display_clear()
{
    write_reg(0x01);
}

void display_cursor(uint8_t loc)
{
    write_reg(0x80 | loc);
}

void display_print(const char *msg)
{
    display_cursor(0x00);
    for ( ; *msg; ++msg) {
        if (*msg == '\n') {
            display_cursor(0x40);
        } else {
            write_data((uint8_t)(*msg));
        }
    }
}

void display_reset()
{
    OUT(RS);
    CCR1(RS);
    CCR2(RS);
    CLR(RS);
    OUT(RW);
    CCR1(RW);
    CCR2(RW);
    CLR(RW);
    OUT(EN);
    SCR1(EN);
    CCR2(EN);
    CLR(EN);
    OUT_SPAN(DISPLAY);
    delay_ms(40);

    CLR(RS);
    write_half(0x03);
    delay_ms(5);
    write_half(0x03);
    delay_us(150);
    write_half(0x03);
    delay_us(150);
    write_half(0x02);
    delay_us(150);
    // function set 00 001D_NFxx
    // where:
    // D = 8-bit mode (4-bit)
    // N = 2-line mode (1-line)
    // F = 5x11 dots (5x8)
    write_reg(0x28);  // set mode
    write_reg(0x08);  // display off
    write_reg(0x01);  // clear display
    delay_ms(2);
    write_reg(0x06);  // entry mode set
    write_reg(0x0c);  // display on, cursor off, blink off
}
