#ifndef GPIB_H
#define GPIB_H
#include <stdint.h>
#include "stm8.h"

/*
 * this ISR declaration needs to be here and this header included
 * in main.c; otherwise, SDCC will not put it into the vector table.
 */
void atn_isr(void) __interrupt(EXTI1_ISR);
void dav_isr(void) __interrupt(EXTI0_ISR);
void ndac_isr(void) __interrupt(EXTI2_ISR);
void gpib_init();
uint8_t gpib_eoi();
uint8_t gpib_atn();
uint8_t gpib_dav();
uint8_t gpib_nrfd();
uint8_t gpib_ndac();
uint8_t gpib_dataread();
uint8_t gpib_address();
uint32_t gpib_intcount();

void gpib_set_save_callback(int (*cb)(int));

void gpib_set_listener_mode(void);
void gpib_send_byte_talker(uint8_t byte, uint8_t eoi);

enum bus_state {
                idle,
                listening,
                filename,
                unlisten,
                lagsag,
                scg,
                load,
                save
            };
enum bus_state gpib_state();
const char *gpib_filename();
bool gpib_acked();
void gpib_dump();
bool gpib_data_dump(uint8_t byte);

#endif // GPIB_H
