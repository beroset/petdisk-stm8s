#ifndef DISPLAY_H
#define DISPLAY_H
#include <stdint.h>

void display_reset(); 
void display_cursor(uint8_t loc);
void display_print(const char *msg);
void display_clear();
uint8_t display_readaddr();

#endif // DISPLAY_H
