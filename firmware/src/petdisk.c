/* PetDisk top-level application wiring. */
#include "petdisk.h"

void PetDisk_init(PetDisk* self,
                  IGpioPort* dataPort,
                  IGpioPin* dav, IGpioPin* nrfd, IGpioPin* ndac,
                  IGpioPin* atn, IGpioPin* eoi, IGpioPin* srq,
                  IGpioPin* ifc, IGpioPin* ren,
                  ISpi* spi, ITimer* timer,
                  IFilesystem* fs,
                  uint8_t deviceAddress)
{
    Ieee488_init(&self->ieee, dataPort, dav, nrfd, ndac,
                 atn, eoi, srq, ifc, ren, deviceAddress);
    SdCard_init(&self->sd, spi, timer);
    CbmDos_init(&self->dos, &self->ieee, fs);
}

void PetDisk_begin(PetDisk* self)
{
    Ieee488_begin(&self->ieee);
    SdCard_begin(&self->sd);
    CbmDos_reset(&self->dos);
}

void PetDisk_run(PetDisk* self)
{
    Ieee488_poll(&self->ieee);
}
