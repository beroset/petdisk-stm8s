#pragma once
/* STM8 HAL initialisation helper.
 * Declared only when compiling with SDCC for the STM8 target.
 */
#ifdef __SDCC
#include "hal.h"

/* Initialise all static STM8 HAL objects and return pointers to them.
 * Call this once from main() before constructing PetDisk. */
void stm8_hal_init(IGpioPort** dataPort,
                   IGpioPin**  dav,   IGpioPin** nrfd, IGpioPin** ndac,
                   IGpioPin**  atn,   IGpioPin** eoi,  IGpioPin** srq,
                   IGpioPin**  ifc,   IGpioPin** ren,
                   ISpi**      spi,
                   ITimer**    timer);
#endif /* __SDCC */
