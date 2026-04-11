#pragma once
// HAL abstract interfaces for PetDisk firmware.
// Implemented by hal_linux.cpp (simulation/test) and hal_stm8.cpp (target).
#include <cstdint>

// GPIO single-pin abstraction
class IGpioPin {
public:
    virtual ~IGpioPin() = default;
    virtual void setOutput() = 0;
    virtual void setInput() = 0;
    virtual void setOpenDrain() = 0;  // output that can only pull low
    virtual void setHigh() = 0;       // release (open-drain: float high via pull-up)
    virtual void setLow() = 0;        // assert (open-drain: pull to GND)
    virtual bool read() const = 0;    // true = HIGH
};

// 8-bit GPIO port abstraction (used for IEEE-488 data bus)
class IGpioPort {
public:
    virtual ~IGpioPort() = default;
    virtual void setOutputMask(uint8_t mask) = 0;
    virtual void setInputMask(uint8_t mask) = 0;
    virtual void setOpenDrainMask(uint8_t mask) = 0;
    virtual void write(uint8_t value) = 0;
    virtual uint8_t read() const = 0;
};

// SPI bus abstraction (used for SD card)
class ISpi {
public:
    virtual ~ISpi() = default;
    virtual void init() = 0;
    virtual uint8_t transfer(uint8_t data) = 0;
    virtual void csAssert() = 0;
    virtual void csDeassert() = 0;
};

// Timer / delay abstraction
class ITimer {
public:
    virtual ~ITimer() = default;
    virtual void delayMs(uint32_t ms) = 0;
    virtual void delayUs(uint32_t us) = 0;
    virtual uint32_t tickMs() const = 0;
};
