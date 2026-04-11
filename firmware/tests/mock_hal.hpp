#pragma once
// Mock HAL for unit tests – thin wrappers around in-memory state.
// Provides injectable pin/port/SPI/timer values for deterministic tests.
#include "hal.hpp"
#include "cbmdos.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>

// ── MockGpioPin ───────────────────────────────────────────────────────────────

class MockGpioPin : public IGpioPin {
public:
    void setOutput()    override { isInput = false; isOpenDrain = false; }
    void setInput()     override { isInput = true;  }
    void setOpenDrain() override { isInput = false; isOpenDrain = true;  }
    void setHigh()      override { if (!isInput) driven = true;  }
    void setLow()       override { if (!isInput) driven = false; }

    bool read() const override {
        return injected.has_value() ? *injected : driven;
    }

    // Test helpers
    void inject(bool v)       { injected = v; }
    void clearInjection()     { injected.reset(); }
    bool outputState() const  { return driven; }
    bool inputMode()   const  { return isInput; }
    bool openDrainMode() const { return isOpenDrain; }

    bool isInput{false};
    bool isOpenDrain{false};
    bool driven{true};                   // default: high (released)
    std::optional<bool> injected{};
};

// ── MockGpioPort ──────────────────────────────────────────────────────────────

class MockGpioPort : public IGpioPort {
public:
    void setOutputMask(uint8_t mask)   override { inputMask &= ~mask; }
    void setInputMask(uint8_t mask)    override { inputMask |= mask;  }
    void setOpenDrainMask(uint8_t mask) override { inputMask &= ~mask; odMask |= mask; }
    void write(uint8_t value)          override { output = value; }
    uint8_t read() const               override {
        // Wired-AND: both our drive and external bus can pull lines low.
        // With busValue=0xFF (default), read() returns output.
        // When test injects a bus value, the AND models real open-drain behaviour.
        return output & busValue;
    }

    // Test helpers
    void injectBus(uint8_t v)        { busValue = v; }
    uint8_t outputValue() const      { return output; }

    uint8_t inputMask{0xFF};
    uint8_t odMask{0};
    uint8_t output{0xFF};
    uint8_t busValue{0xFF};
};

// ── MockSpi ───────────────────────────────────────────────────────────────────

class MockSpi : public ISpi {
public:
    // init() on mock intentionally does NOT reset the queue, so tests can
    // pre-populate the queue before calling the driver's init().
    void init() override {}

    uint8_t transfer(uint8_t data) override {
        txLog.push_back(data);
        if (rxIndex < rxQueue.size()) {
            return rxQueue[rxIndex++];
        }
        return 0xFF;
    }

    void csAssert()   override { csState = true;  }
    void csDeassert() override { csState = false; }

    // Test helpers – queue bytes to return from transfer()
    void queueRx(uint8_t byte)                      { rxQueue.push_back(byte); }
    void queueRx(const std::vector<uint8_t>& bytes) {
        rxQueue.insert(rxQueue.end(), bytes.begin(), bytes.end());
    }
    void reset() {
        txLog.clear();
        rxQueue.clear();
        rxIndex = 0;
        csState = false;
    }

    std::vector<uint8_t> txLog{};
    std::vector<uint8_t> rxQueue{};
    size_t               rxIndex{0};
    bool                 csState{false};
};

// ── MockTimer ─────────────────────────────────────────────────────────────────

class MockTimer : public ITimer {
public:
    void delayMs(uint32_t ms) override { elapsed += ms; }
    void delayUs(uint32_t)    override {}
    uint32_t tickMs() const   override { return elapsed; }

    uint32_t elapsed{0};
};

// ── MockFilesystem (for CbmDos tests) ────────────────────────────────────────

class MockFilesystem : public CbmDos::IFilesystem {
public:
    struct OpenCall { uint8_t channel; std::string filename; uint8_t mode; };

    bool open(uint8_t channel, const char* filename, uint8_t mode) override {
        openCalls.push_back({channel, filename, mode});
        return openResult;
    }
    void close(uint8_t channel) override {
        closedChannels.push_back(channel);
    }
    size_t read(uint8_t, uint8_t* buf, size_t maxLen) override {
        if (readData.empty()) return 0;
        size_t n = std::min(maxLen, readData.size());
        std::memcpy(buf, readData.data(), n);
        return n;
    }
    size_t write(uint8_t, const uint8_t* buf, size_t len) override {
        writtenData.insert(writtenData.end(), buf, buf + len);
        return len;
    }
    uint8_t     errorCode()    const override { return lastErrorCode; }
    const char* errorMessage() const override { return lastErrorMsg; }
    void executeCommand(const char* cmd, size_t len) override {
        lastCommand.assign(cmd, len);
    }

    // Test controls
    bool                   openResult{true};
    uint8_t                lastErrorCode{0};
    const char*            lastErrorMsg{"OK"};
    std::vector<OpenCall>  openCalls{};
    std::vector<uint8_t>   closedChannels{};
    std::vector<uint8_t>   readData{};
    std::vector<uint8_t>   writtenData{};
    std::string            lastCommand{};
};
