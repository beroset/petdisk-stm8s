// Linux simulation HAL – in-memory implementation for testing and host simulation.
// Implements the C vtable (hal.h) interface using C++ classes.
// Each class embeds the C interface struct as its FIRST member so that a
// pointer to the class may be safely cast to the interface pointer type.
#include "hal.h"

#include <chrono>
#include <thread>
#include <cstring>
#include <optional>
#include <vector>

// ── LinuxGpioPin ─────────────────────────────────────────────────────────────

struct LinuxGpioPin {
    IGpioPin iface;   // MUST be first
    bool     isInput{false};
    bool     isOpenDrain{false};
    bool     value{false};
    std::optional<bool> externalValue{};

    LinuxGpioPin() {
        iface.setOutput    = [](IGpioPin* p) { self(p)->isInput = false; self(p)->isOpenDrain = false; };
        iface.setInput     = [](IGpioPin* p) { self(p)->isInput = true; };
        iface.setOpenDrain = [](IGpioPin* p) { self(p)->isInput = false; self(p)->isOpenDrain = true; };
        iface.setHigh      = [](IGpioPin* p) { if (!self(p)->isInput) self(p)->value = true; };
        iface.setLow       = [](IGpioPin* p) { if (!self(p)->isInput) self(p)->value = false; };
        iface.read         = [](const IGpioPin* p) -> int {
            const LinuxGpioPin* s = reinterpret_cast<const LinuxGpioPin*>(p);
            return (s->externalValue.has_value() ? *s->externalValue : s->value) ? 1 : 0;
        };
    }

    IGpioPin* pin() { return &iface; }

    // Test helpers
    void injectValue(bool v) { externalValue = v; }
    void clearInjection()    { externalValue.reset(); }
    bool outputValue() const { return value; }

private:
    static LinuxGpioPin* self(IGpioPin* p) {
        return reinterpret_cast<LinuxGpioPin*>(p);
    }
};

// ── LinuxGpioPort ─────────────────────────────────────────────────────────────

struct LinuxGpioPort {
    IGpioPort iface;   // MUST be first
    uint8_t   inputMask{0xFF};
    uint8_t   odMask{0};
    uint8_t   output{0xFF};
    uint8_t   injected{0xFF};

    LinuxGpioPort() {
        iface.setOutputMask    = [](IGpioPort* p, uint8_t m) { self(p)->inputMask &= ~m; };
        iface.setInputMask     = [](IGpioPort* p, uint8_t m) { self(p)->inputMask |=  m; };
        iface.setOpenDrainMask = [](IGpioPort* p, uint8_t m) { self(p)->inputMask &= ~m; self(p)->odMask |= m; };
        iface.write            = [](IGpioPort* p, uint8_t v) { self(p)->output = v; };
        iface.read             = [](const IGpioPort* p) -> uint8_t {
            const LinuxGpioPort* s = reinterpret_cast<const LinuxGpioPort*>(p);
            return (s->injected & s->inputMask) | (s->output & ~s->inputMask);
        };
    }

    IGpioPort* port() { return &iface; }

    // Test helpers
    void injectBus(uint8_t v) { injected = v; }
    uint8_t outputValue() const { return output; }

private:
    static LinuxGpioPort* self(IGpioPort* p) {
        return reinterpret_cast<LinuxGpioPort*>(p);
    }
};

// ── LinuxSpi ─────────────────────────────────────────────────────────────────

struct LinuxSpi {
    ISpi                  iface;   // MUST be first
    std::vector<uint8_t>  txLog{};
    std::vector<uint8_t>  rxQueue{};
    size_t                rxIndex{0};
    bool                  csState{false};

    LinuxSpi() {
        iface.init      = [](ISpi* p) {
            LinuxSpi* s = self(p);
            s->txLog.clear(); s->rxQueue.clear(); s->rxIndex = 0; s->csState = false;
        };
        iface.transfer  = [](ISpi* p, uint8_t data) -> uint8_t {
            LinuxSpi* s = self(p);
            s->txLog.push_back(data);
            if (s->rxIndex < s->rxQueue.size()) return s->rxQueue[s->rxIndex++];
            return 0xFF;
        };
        iface.csAssert   = [](ISpi* p) { self(p)->csState = true; };
        iface.csDeassert = [](ISpi* p) { self(p)->csState = false; };
    }

    ISpi* spi() { return &iface; }

    // Test helpers
    void queueRx(uint8_t byte)                      { rxQueue.push_back(byte); }
    void queueRx(const std::vector<uint8_t>& bytes) {
        rxQueue.insert(rxQueue.end(), bytes.begin(), bytes.end());
    }
    void reset() { txLog.clear(); rxQueue.clear(); rxIndex = 0; }
    bool csAsserted() const { return csState; }

private:
    static LinuxSpi* self(ISpi* p) {
        return reinterpret_cast<LinuxSpi*>(p);
    }
};

// ── LinuxTimer ────────────────────────────────────────────────────────────────

struct LinuxTimer {
    ITimer iface;   // MUST be first

    LinuxTimer() {
        iface.delayMs = [](ITimer*, uint32_t ms) {
            std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        };
        iface.delayUs = [](ITimer*, uint32_t us) {
            std::this_thread::sleep_for(std::chrono::microseconds(us));
        };
        iface.tickMs  = [](const ITimer*) -> uint32_t {
            using namespace std::chrono;
            auto now = steady_clock::now().time_since_epoch();
            return static_cast<uint32_t>(duration_cast<milliseconds>(now).count());
        };
    }

    ITimer* timer() { return &iface; }
};
