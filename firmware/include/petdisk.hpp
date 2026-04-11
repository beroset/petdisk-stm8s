#pragma once
// Top-level application for PetDisk.
// Wires together the HAL, IEEE-488 driver, SD card driver and CBM DOS handler.
#include "hal.hpp"
#include "ieee488.hpp"
#include "sdcard.hpp"
#include "cbmdos.hpp"

class PetDisk {
public:
    PetDisk(IGpioPort& dataPort,
            IGpioPin& dav, IGpioPin& nrfd, IGpioPin& ndac,
            IGpioPin& atn, IGpioPin& eoi, IGpioPin& srq,
            IGpioPin& ifc, IGpioPin& ren,
            ISpi& spi, ITimer& timer,
            CbmDos::IFilesystem& fs,
            uint8_t deviceAddress = 8);

    // Call once at startup.
    void init();

    // Call repeatedly from main loop.
    void run();

private:
    Ieee488 m_ieee;
    SdCard  m_sd;
    CbmDos  m_dos;
};
