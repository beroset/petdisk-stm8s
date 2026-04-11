#pragma once
// Mock HAL for unit tests – thin C++ wrappers around in-memory state.
// Each mock struct embeds the C interface struct as its FIRST member so that
// a pointer to the mock may be cast to the interface pointer type.
// Provides injectable pin/port/SPI/timer/filesystem values for deterministic tests.
#include "hal.h"
#include "cbmdos.h"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>

// ── MockGpioPin ───────────────────────────────────────────────────────────────

struct MockGpioPin {
    IGpioPin            iface;        // MUST be first
    bool                isInput{false};
    bool                isOpenDrain{false};
    bool                driven{true};  // default: high (released)
    std::optional<bool> injected{};

    MockGpioPin() {
        iface.setOutput    = [](IGpioPin* p) { s(p)->isInput = false; s(p)->isOpenDrain = false; };
        iface.setInput     = [](IGpioPin* p) { s(p)->isInput = true; };
        iface.setOpenDrain = [](IGpioPin* p) { s(p)->isInput = false; s(p)->isOpenDrain = true; };
        iface.setHigh      = [](IGpioPin* p) { if (!s(p)->isInput) s(p)->driven = true; };
        iface.setLow       = [](IGpioPin* p) { if (!s(p)->isInput) s(p)->driven = false; };
        iface.read         = [](const IGpioPin* p) -> int {
            const MockGpioPin* self = reinterpret_cast<const MockGpioPin*>(p);
            return (self->injected.has_value() ? *self->injected : self->driven) ? 1 : 0;
        };
    }

    // Expose C interface pointer
    IGpioPin* pin() { return &iface; }

    // Test helpers
    void inject(bool v)         { injected = v; }
    void clearInjection()       { injected.reset(); }
    bool outputState() const    { return driven; }
    bool inputMode()   const    { return isInput; }
    bool openDrainMode() const  { return isOpenDrain; }

private:
    static MockGpioPin* s(IGpioPin* p) { return reinterpret_cast<MockGpioPin*>(p); }
};

// ── MockGpioPort ──────────────────────────────────────────────────────────────

struct MockGpioPort {
    IGpioPort iface;          // MUST be first
    uint8_t   inputMask{0xFF};
    uint8_t   odMask{0};
    uint8_t   output{0xFF};
    uint8_t   busValue{0xFF};

    MockGpioPort() {
        iface.setOutputMask    = [](IGpioPort* p, uint8_t m) { s(p)->inputMask &= ~m; };
        iface.setInputMask     = [](IGpioPort* p, uint8_t m) { s(p)->inputMask |=  m; };
        iface.setOpenDrainMask = [](IGpioPort* p, uint8_t m) { s(p)->inputMask &= ~m; s(p)->odMask |= m; };
        iface.write            = [](IGpioPort* p, uint8_t v) { s(p)->output = v; };
        iface.read             = [](const IGpioPort* p) -> uint8_t {
            const MockGpioPort* self = reinterpret_cast<const MockGpioPort*>(p);
            // Wired-AND: both our drive and external bus can pull lines low.
            return self->output & self->busValue;
        };
    }

    IGpioPort* port() { return &iface; }

    // Test helpers
    void injectBus(uint8_t v)    { busValue = v; }
    uint8_t outputValue() const  { return output; }

private:
    static MockGpioPort* s(IGpioPort* p) { return reinterpret_cast<MockGpioPort*>(p); }
};

// ── MockSpi ───────────────────────────────────────────────────────────────────

struct MockSpi {
    ISpi                  iface;      // MUST be first
    std::vector<uint8_t>  txLog{};
    std::vector<uint8_t>  rxQueue{};
    size_t                rxIndex{0};
    bool                  csState{false};

    MockSpi() {
        // init() on mock intentionally does NOT reset the queue, so tests can
        // pre-populate the queue before calling the driver's init().
        iface.init      = [](ISpi*) {};
        iface.transfer  = [](ISpi* p, uint8_t data) -> uint8_t {
            MockSpi* self = s(p);
            self->txLog.push_back(data);
            if (self->rxIndex < self->rxQueue.size())
                return self->rxQueue[self->rxIndex++];
            return 0xFF;
        };
        iface.csAssert   = [](ISpi* p) { s(p)->csState = true; };
        iface.csDeassert = [](ISpi* p) { s(p)->csState = false; };
    }

    ISpi* spi() { return &iface; }

    // Test helpers – queue bytes to return from transfer()
    void queueRx(uint8_t byte)                      { rxQueue.push_back(byte); }
    void queueRx(const std::vector<uint8_t>& bytes) {
        rxQueue.insert(rxQueue.end(), bytes.begin(), bytes.end());
    }
    void reset() {
        txLog.clear(); rxQueue.clear(); rxIndex = 0; csState = false;
    }

private:
    static MockSpi* s(ISpi* p) { return reinterpret_cast<MockSpi*>(p); }
};

// ── MockTimer ─────────────────────────────────────────────────────────────────

struct MockTimer {
    ITimer   iface;      // MUST be first
    uint32_t elapsed{0};

    MockTimer() {
        iface.delayMs = [](ITimer* p, uint32_t ms) { s(p)->elapsed += ms; };
        iface.delayUs = [](ITimer*, uint32_t) {};
        iface.tickMs  = [](const ITimer* p) -> uint32_t {
            return reinterpret_cast<const MockTimer*>(p)->elapsed;
        };
    }

    ITimer* timer() { return &iface; }

private:
    static MockTimer* s(ITimer* p) { return reinterpret_cast<MockTimer*>(p); }
};

// ── MockFilesystem (for CbmDos tests) ────────────────────────────────────────

struct MockFilesystem {
    IFilesystem            iface;      // MUST be first
    struct OpenCall { uint8_t channel; std::string filename; uint8_t mode; };

    bool                   openResult{true};
    uint8_t                lastErrorCode{0};
    const char*            lastErrorMsg{"OK"};
    std::vector<OpenCall>  openCalls{};
    std::vector<uint8_t>   closedChannels{};
    std::vector<uint8_t>   readData{};
    std::vector<uint8_t>   writtenData{};
    std::string            lastCommand{};

    MockFilesystem() {
        iface.open = [](IFilesystem* p, uint8_t ch, const char* fn, uint8_t mode) -> int {
            MockFilesystem* self = s(p);
            self->openCalls.push_back({ch, fn, mode});
            return self->openResult ? 1 : 0;
        };
        iface.close = [](IFilesystem* p, uint8_t ch) {
            s(p)->closedChannels.push_back(ch);
        };
        iface.read = [](IFilesystem* p, uint8_t, uint8_t* buf, size_t maxLen) -> size_t {
            MockFilesystem* self = s(p);
            if (self->readData.empty()) return 0;
            size_t n = std::min(maxLen, self->readData.size());
            std::memcpy(buf, self->readData.data(), n);
            return n;
        };
        iface.write = [](IFilesystem* p, uint8_t, const uint8_t* buf, size_t len) -> size_t {
            MockFilesystem* self = s(p);
            self->writtenData.insert(self->writtenData.end(), buf, buf + len);
            return len;
        };
        iface.errorCode    = [](const IFilesystem* p) -> uint8_t {
            return reinterpret_cast<const MockFilesystem*>(p)->lastErrorCode;
        };
        iface.errorMessage = [](const IFilesystem* p) -> const char* {
            return reinterpret_cast<const MockFilesystem*>(p)->lastErrorMsg;
        };
        iface.executeCommand = [](IFilesystem* p, const char* cmd, size_t len) {
            s(p)->lastCommand.assign(cmd, len);
        };
    }

    IFilesystem* fs() { return &iface; }

private:
    static MockFilesystem* s(IFilesystem* p) { return reinterpret_cast<MockFilesystem*>(p); }
};
