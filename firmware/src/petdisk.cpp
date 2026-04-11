// PetDisk top-level application wiring.
#include "petdisk.hpp"

PetDisk::PetDisk(IGpioPort& dataPort,
                 IGpioPin& dav, IGpioPin& nrfd, IGpioPin& ndac,
                 IGpioPin& atn, IGpioPin& eoi, IGpioPin& srq,
                 IGpioPin& ifc, IGpioPin& ren,
                 ISpi& spi, ITimer& timer,
                 CbmDos::IFilesystem& fs,
                 uint8_t deviceAddress)
    : m_ieee(dataPort, dav, nrfd, ndac, atn, eoi, srq, ifc, ren, deviceAddress),
      m_sd(spi, timer),
      m_dos(m_ieee, fs)
{}

void PetDisk::init() {
    m_ieee.init();
    m_sd.init();
    m_dos.reset();
}

void PetDisk::run() {
    m_ieee.poll();
}
