#pragma once
// IEEE-488 / GPIB bus driver for PetDisk.
// Implements the three-wire handshake as a listener/talker device.
// All IEEE-488 lines are active-LOW; open-drain output is used throughout.
#include "hal.hpp"
#include <cstdint>
#include <cstddef>

class Ieee488 {
public:
    // Callback invoked when a data byte arrives while addressed as listener.
    using DataCallback = void (*)(uint8_t byte, bool eoi, void* ctx);

    // Callback invoked when the controller wants to read a byte (talker mode).
    // Return value: byte to place on bus; set *isLast=true to assert EOI.
    using TalkCallback = int (*)(bool* isLast, void* ctx);  // -1 = no data

    Ieee488(IGpioPort& dataBus,
            IGpioPin& dav,
            IGpioPin& nrfd,
            IGpioPin& ndac,
            IGpioPin& atn,
            IGpioPin& eoi,
            IGpioPin& srq,
            IGpioPin& ifc,
            IGpioPin& ren,
            uint8_t deviceAddress);

    void init();

    // Must be called repeatedly from the main loop.
    void poll();

    // Send a single byte as talker (blocks until handshake completes or timeout).
    // isLastByte: if true, EOI is asserted with this byte.
    // Returns true on success.
    bool sendByte(uint8_t byte, bool isLastByte);

    // Receive a single byte as listener.
    // Returns byte value (0-255) or -1 on error/timeout.
    // isLastByte is set true if EOI was asserted with this byte.
    int receiveByte(bool& isLastByte);

    bool isAddressedToListen() const { return m_listenActive; }
    bool isAddressedToTalk()   const { return m_talkActive; }
    bool isAttnActive()        const;

    void assertSrq();
    void releaseSrq();

    void setDataCallback(DataCallback cb, void* ctx) {
        m_dataCb  = cb;
        m_dataCbCtx = ctx;
    }
    void setTalkCallback(TalkCallback cb, void* ctx) {
        m_talkCb    = cb;
        m_talkCbCtx = ctx;
    }

    // Timeout limit (loop iterations) for handshake operations.
    // Allows adjustment without platform-specific timing.
    static constexpr uint32_t kDefaultTimeout = 100'000u;

private:
    // Helpers
    void handleAttn();
    void releaseDataBus();
    void driveDataBus(uint8_t byte);

    IGpioPort& m_dataBus;
    IGpioPin&  m_dav;
    IGpioPin&  m_nrfd;
    IGpioPin&  m_ndac;
    IGpioPin&  m_atn;
    IGpioPin&  m_eoi;
    IGpioPin&  m_srq;
    IGpioPin&  m_ifc;
    IGpioPin&  m_ren;

    uint8_t m_deviceAddress;
    bool    m_listenActive{false};
    bool    m_talkActive{false};

    DataCallback m_dataCb{nullptr};
    void*        m_dataCbCtx{nullptr};
    TalkCallback m_talkCb{nullptr};
    void*        m_talkCbCtx{nullptr};
};
