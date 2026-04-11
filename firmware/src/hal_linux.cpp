// Linux simulation HAL – in-memory implementation for testing and host simulation.
#include "hal.hpp"

#include <chrono>
#include <thread>
#include <cstring>
#include <vector>

// ── LinuxGpioPin ─────────────────────────────────────────────────────────────

class LinuxGpioPin : public IGpioPin {
public:
    void setOutput()    override { m_isInput = false; m_isOpenDrain = false; }
    void setInput()     override { m_isInput = true;  }
    void setOpenDrain() override { m_isInput = false; m_isOpenDrain = true;  }
    void setHigh()      override { if (!m_isInput) m_value = true;  }
    void setLow()       override { if (!m_isInput) m_value = false; }
    bool read()   const override { return m_externalValue.has_value()
                                          ? *m_externalValue : m_value; }

    // Test helpers
    void injectValue(bool v) { m_externalValue = v; }
    void clearInjection()    { m_externalValue.reset(); }
    bool outputValue() const { return m_value; }

private:
    bool m_isInput{false};
    bool m_isOpenDrain{false};
    bool m_value{false};
    std::optional<bool> m_externalValue{};
};

// ── LinuxGpioPort ─────────────────────────────────────────────────────────────

class LinuxGpioPort : public IGpioPort {
public:
    void setOutputMask(uint8_t mask) override   { m_inputMask &= ~mask; }
    void setInputMask(uint8_t mask) override    { m_inputMask |= mask;  }
    void setOpenDrainMask(uint8_t mask) override { m_inputMask &= ~mask; m_odMask |= mask; }
    void write(uint8_t value) override           { m_output = value; }
    uint8_t read() const override {
        // Input bits come from injected value; output bits come from m_output
        return (m_injected & m_inputMask) | (m_output & ~m_inputMask);
    }

    // Test helpers
    void injectBus(uint8_t value) { m_injected = value; }
    uint8_t outputValue() const   { return m_output; }

private:
    uint8_t m_inputMask{0xFF};
    uint8_t m_odMask{0};
    uint8_t m_output{0xFF};
    uint8_t m_injected{0xFF};
};

// ── LinuxSpi ─────────────────────────────────────────────────────────────────

class LinuxSpi : public ISpi {
public:
    void init() override {
        m_txLog.clear();
        m_rxQueue.clear();
        m_rxIndex = 0;
        m_csAsserted = false;
    }

    uint8_t transfer(uint8_t data) override {
        m_txLog.push_back(data);
        if (m_rxIndex < m_rxQueue.size()) {
            return m_rxQueue[m_rxIndex++];
        }
        return 0xFF;  // default: MISO high
    }

    void csAssert()   override { m_csAsserted = true;  }
    void csDeassert() override { m_csAsserted = false; }

    // Test helpers
    void queueRx(const std::vector<uint8_t>& bytes) {
        m_rxQueue.insert(m_rxQueue.end(), bytes.begin(), bytes.end());
    }
    void queueRx(uint8_t byte) { m_rxQueue.push_back(byte); }

    const std::vector<uint8_t>& txLog() const { return m_txLog; }
    void clearTx() { m_txLog.clear(); }
    bool csAsserted() const { return m_csAsserted; }

    // Reset RX queue and TX log
    void reset() {
        m_txLog.clear();
        m_rxQueue.clear();
        m_rxIndex = 0;
    }

private:
    std::vector<uint8_t> m_txLog{};
    std::vector<uint8_t> m_rxQueue{};
    size_t               m_rxIndex{0};
    bool                 m_csAsserted{false};
};

// ── LinuxTimer ────────────────────────────────────────────────────────────────

class LinuxTimer : public ITimer {
public:
    void delayMs(uint32_t ms) override {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
    void delayUs(uint32_t us) override {
        std::this_thread::sleep_for(std::chrono::microseconds(us));
    }
    uint32_t tickMs() const override {
        using namespace std::chrono;
        auto now = steady_clock::now().time_since_epoch();
        return static_cast<uint32_t>(duration_cast<milliseconds>(now).count());
    }
};
