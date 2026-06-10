#ifndef SPI_H
#define SPI_H

#include <stdint.h>

void SPI_init();
void SPI_write(uint8_t data);
uint8_t SPI_read();
void chip_select();
void chip_deselect();

#endif // SPI_H
