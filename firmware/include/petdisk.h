#pragma once
/* Top-level application for PetDisk.
 * Wires together the HAL, IEEE-488 driver, SD card driver and CBM DOS handler.
 */
#include "hal.h"
#include "ieee488.h"
#include "sdcard.h"
#include "cbmdos.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    Ieee488 ieee;
    SdCard  sd;
    CbmDos  dos;
} PetDisk;

/* Initialise the PetDisk struct with all required HAL objects. */
void PetDisk_init(PetDisk* self,
                  IGpioPort* dataPort,
                  IGpioPin* dav, IGpioPin* nrfd, IGpioPin* ndac,
                  IGpioPin* atn, IGpioPin* eoi, IGpioPin* srq,
                  IGpioPin* ifc, IGpioPin* ren,
                  ISpi* spi, ITimer* timer,
                  IFilesystem* fs,
                  uint8_t deviceAddress);

/* Call once at startup: configure all hardware. */
void PetDisk_begin(PetDisk* self);

/* Call repeatedly from the main loop. */
void PetDisk_run(PetDisk* self);

#ifdef __cplusplus
} /* extern "C" */
#endif
