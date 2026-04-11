// IEEE-488 / GPIB three-wire handshake driver for PetDisk.
// All bus signals are active-LOW; open-drain outputs are used throughout.
// "assert" means drive LOW; "release" means float HIGH (open-drain).
#include "ieee488.hpp"
#include <cstddef>

// IEEE-488 listen/talk address offsets (added to the device address)
static constexpr uint8_t kListenAddr = 0x20;  // MLA: My Listen Address
static constexpr uint8_t kTalkAddr   = 0x40;  // MTA: My Talk Address
static constexpr uint8_t kUnlisten   = 0x3F;
static constexpr uint8_t kUntalk     = 0x5F;

Ieee488::Ieee488(IGpioPort& dataBus,
                 IGpioPin& dav,
                 IGpioPin& nrfd,
                 IGpioPin& ndac,
                 IGpioPin& atn,
                 IGpioPin& eoi,
                 IGpioPin& srq,
                 IGpioPin& ifc,
                 IGpioPin& ren,
                 uint8_t deviceAddress)
    : m_dataBus(dataBus),
      m_dav(dav), m_nrfd(nrfd), m_ndac(ndac),
      m_atn(atn), m_eoi(eoi), m_srq(srq),
      m_ifc(ifc), m_ren(ren),
      m_deviceAddress(deviceAddress)
{}

void Ieee488::init() {
    // Configure control lines
    m_dav.setOpenDrain();  m_dav.setHigh();   // release DAV
    m_nrfd.setOpenDrain(); m_nrfd.setHigh();  // release NRFD
    m_ndac.setOpenDrain(); m_ndac.setHigh();  // release NDAC
    m_eoi.setOpenDrain();  m_eoi.setHigh();   // release EOI
    m_srq.setOpenDrain();  m_srq.setHigh();   // release SRQ

    // ATN, IFC, REN are controller outputs – we only listen
    m_atn.setInput();
    m_ifc.setInput();
    m_ren.setInput();

    // Data bus: open-drain, release all lines
    m_dataBus.setOpenDrainMask(0xFF);
    m_dataBus.write(0xFF);  // all HIGH = release
}

bool Ieee488::isAttnActive() const {
    return !m_atn.read();  // active LOW
}

void Ieee488::assertSrq() {
    m_srq.setLow();
}

void Ieee488::releaseSrq() {
    m_srq.setHigh();
}

// Release (float) all data lines so another talker can use the bus.
void Ieee488::releaseDataBus() {
    m_dataBus.write(0xFF);
}

// Drive inverted data value onto open-drain bus.
// IEEE-488 data lines are active-LOW: a '1' bit = line LOW, '0' bit = line HIGH.
void Ieee488::driveDataBus(uint8_t byte) {
    m_dataBus.write(~byte);
}

// Handle ATN commands sent by the controller.
// ATN commands are single bytes:
//   0x20-0x3E  MLA (My Listen Address)
//   0x3F       Unlisten
//   0x40-0x5E  MTA (My Talk Address)
//   0x5F       Untalk
//   0x60-0x6F  Secondary address
void Ieee488::handleAttn() {
    bool lastByte = false;
    int b = receiveByte(lastByte);
    if (b < 0) return;

    uint8_t cmd = static_cast<uint8_t>(b);

    if (cmd == kUnlisten) {
        m_listenActive = false;
        releaseDataBus();
        return;
    }
    if (cmd == kUntalk) {
        m_talkActive = false;
        releaseDataBus();
        return;
    }
    if (cmd == (kListenAddr | (m_deviceAddress & 0x1F))) {
        m_listenActive = true;
        m_talkActive   = false;
        return;
    }
    if (cmd == (kTalkAddr | (m_deviceAddress & 0x1F))) {
        m_talkActive   = true;
        m_listenActive = false;
        return;
    }
    // Other addresses or secondary addresses – not for us, ignore.
}

// poll() is called repeatedly from the main loop.
// When ATN is asserted, we handle the command byte.
// When addressed as listener, receive data bytes and invoke callback.
// When addressed as talker, send data bytes via callback.
void Ieee488::poll() {
    // IFC resets the device
    if (!m_ifc.read()) {
        m_listenActive = false;
        m_talkActive   = false;
        releaseDataBus();
        m_nrfd.setHigh();
        m_ndac.setHigh();
        return;
    }

    if (isAttnActive()) {
        handleAttn();
        return;
    }

    if (m_listenActive && m_dataCb != nullptr) {
        bool eoi = false;
        int b = receiveByte(eoi);
        if (b >= 0) {
            m_dataCb(static_cast<uint8_t>(b), eoi, m_dataCbCtx);
        }
    }

    if (m_talkActive && m_talkCb != nullptr) {
        bool isLast = false;
        int b = m_talkCb(&isLast, m_talkCbCtx);
        if (b >= 0) {
            sendByte(static_cast<uint8_t>(b), isLast);
        }
    }
}

// Receive one byte using the three-wire handshake (listener side).
// Sequence:
//   1. Assert NDAC (not accepted), release NRFD (ready)
//   2. Wait for DAV LOW (talker has valid data)
//   3. Assert NRFD (not ready), read data, release NDAC (accepted)
//   4. Wait for DAV HIGH (talker acknowledged acceptance)
//   5. Release NRFD
int Ieee488::receiveByte(bool& isLastByte) {
    // Assert NDAC – we have not accepted previous data
    m_ndac.setLow();
    // Release NRFD – we are ready for data
    m_nrfd.setHigh();

    // Wait for DAV LOW (talker asserts data valid)
    uint32_t timeout = kDefaultTimeout;
    while (m_dav.read()) {
        if (--timeout == 0) {
            m_ndac.setHigh();
            return -1;
        }
    }

    // Assert NRFD – not ready for next byte yet
    m_nrfd.setLow();

    // Read data bus (active-LOW on bus, invert for actual data value)
    uint8_t data = ~m_dataBus.read();

    // Check EOI (active LOW)
    isLastByte = !m_eoi.read();

    // Release NDAC – accepted
    m_ndac.setHigh();

    // Wait for DAV to go HIGH (talker has seen our acceptance)
    timeout = kDefaultTimeout;
    while (!m_dav.read()) {
        if (--timeout == 0) {
            return static_cast<int>(data);
        }
    }

    // Release NRFD – ready for next byte
    m_nrfd.setHigh();

    return static_cast<int>(data);
}

// Send one byte using the three-wire handshake (talker side).
// Sequence:
//   1. Wait for NRFD HIGH (listener ready) and NDAC LOW (not yet accepted)
//   2. Place data on bus, optionally assert EOI
//   3. Assert DAV LOW (data valid)
//   4. Wait for NRFD LOW (listener busy) then NDAC HIGH (all accepted)
//   5. Release DAV, release data, release EOI
bool Ieee488::sendByte(uint8_t byte, bool isLastByte) {
    // Wait for NRFD HIGH (all listeners ready)
    uint32_t timeout = kDefaultTimeout;
    while (!m_nrfd.read()) {
        if (--timeout == 0) return false;
    }

    // Place data (inverted for active-LOW bus)
    driveDataBus(byte);

    // Assert EOI if this is the last byte
    if (isLastByte) {
        m_eoi.setLow();
    }

    // Small settling – spin a few cycles (platform-independent)
    for (int i = 0; i < 4; ++i) { asm(""); }

    // Assert DAV – data is valid
    m_dav.setLow();

    // Wait for NRFD LOW (listeners are processing)
    timeout = kDefaultTimeout;
    while (m_nrfd.read()) {
        if (--timeout == 0) {
            m_dav.setHigh();
            releaseDataBus();
            m_eoi.setHigh();
            return false;
        }
    }

    // Wait for NDAC HIGH (all listeners have accepted)
    timeout = kDefaultTimeout;
    while (!m_ndac.read()) {
        if (--timeout == 0) {
            m_dav.setHigh();
            releaseDataBus();
            m_eoi.setHigh();
            return false;
        }
    }

    // Release DAV, data bus, EOI
    m_dav.setHigh();
    releaseDataBus();
    if (isLastByte) {
        m_eoi.setHigh();
    }

    return true;
}
