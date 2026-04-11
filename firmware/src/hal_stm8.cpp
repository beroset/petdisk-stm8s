// STM8S HAL implementation (compiled only with SDCC for the STM8 target).
// Register addresses are for the STM8S105K6T6C.
#ifdef __SDCC

#include "hal.hpp"
#include <cstdint>

// ── STM8S register definitions ───────────────────────────────────────────────
// Port A
#define PA_ODR  (*(volatile uint8_t*)0x5000)
#define PA_IDR  (*(volatile uint8_t*)0x5001)
#define PA_DDR  (*(volatile uint8_t*)0x5002)
#define PA_CR1  (*(volatile uint8_t*)0x5003)
#define PA_CR2  (*(volatile uint8_t*)0x5004)

// Port B
#define PB_ODR  (*(volatile uint8_t*)0x5005)
#define PB_IDR  (*(volatile uint8_t*)0x5006)
#define PB_DDR  (*(volatile uint8_t*)0x5007)
#define PB_CR1  (*(volatile uint8_t*)0x5008)
#define PB_CR2  (*(volatile uint8_t*)0x5009)

// Port C
#define PC_ODR  (*(volatile uint8_t*)0x500A)
#define PC_IDR  (*(volatile uint8_t*)0x500B)
#define PC_DDR  (*(volatile uint8_t*)0x500C)
#define PC_CR1  (*(volatile uint8_t*)0x500D)
#define PC_CR2  (*(volatile uint8_t*)0x500E)

// Port D
#define PD_ODR  (*(volatile uint8_t*)0x500F)
#define PD_IDR  (*(volatile uint8_t*)0x5010)
#define PD_DDR  (*(volatile uint8_t*)0x5011)
#define PD_CR1  (*(volatile uint8_t*)0x5012)
#define PD_CR2  (*(volatile uint8_t*)0x5013)

// SPI1
#define SPI_CR1   (*(volatile uint8_t*)0x5200)
#define SPI_CR2   (*(volatile uint8_t*)0x5201)
#define SPI_ICR   (*(volatile uint8_t*)0x5202)
#define SPI_SR    (*(volatile uint8_t*)0x5203)
#define SPI_DR    (*(volatile uint8_t*)0x5204)

#define SPI_SR_TXE   0x02  // Tx empty
#define SPI_SR_RXNE  0x01  // Rx not empty
#define SPI_SR_BSY   0x80  // Busy

// CLK
#define CLK_CKDIVR (*(volatile uint8_t*)0x50C6)

// TIM2 (used for delay)
#define TIM2_CR1   (*(volatile uint8_t*)0x5300)
#define TIM2_IER   (*(volatile uint8_t*)0x5303)
#define TIM2_SR1   (*(volatile uint8_t*)0x5304)
#define TIM2_EGR   (*(volatile uint8_t*)0x5306)
#define TIM2_CCMR1 (*(volatile uint8_t*)0x5307)
#define TIM2_CNTRH (*(volatile uint8_t*)0x530E)
#define TIM2_CNTRL (*(volatile uint8_t*)0x530F)
#define TIM2_PSCR  (*(volatile uint8_t*)0x5310)
#define TIM2_ARRH  (*(volatile uint8_t*)0x5311)
#define TIM2_ARRL  (*(volatile uint8_t*)0x5312)
#define TIM2_SR1_UIF 0x01

// ── Stm8GpioPin ──────────────────────────────────────────────────────────────

class Stm8GpioPin : public IGpioPin {
public:
    Stm8GpioPin(volatile uint8_t& odr, volatile uint8_t& idr,
                volatile uint8_t& ddr, volatile uint8_t& cr1, uint8_t bit)
        : m_odr(odr), m_idr(idr), m_ddr(ddr), m_cr1(cr1), m_bit(bit) {}

    void setOutput() override {
        m_ddr |= m_bit;   // output
        m_cr1 |= m_bit;   // push-pull
    }
    void setInput() override {
        m_ddr &= ~m_bit;  // input
        m_cr1 &= ~m_bit;  // floating (no pull-up – external pull-ups on IEEE-488)
    }
    // Open-drain: DDR=1, CR1=0
    void setOpenDrain() override {
        m_ddr |= m_bit;
        m_cr1 &= ~m_bit;
    }
    void setHigh() override { m_odr |= m_bit;  }
    void setLow()  override { m_odr &= ~m_bit; }
    bool read() const override { return (m_idr & m_bit) != 0; }

private:
    volatile uint8_t& m_odr;
    volatile uint8_t& m_idr;
    volatile uint8_t& m_ddr;
    volatile uint8_t& m_cr1;
    uint8_t           m_bit;
};

// ── Stm8GpioPort (Port B – full 8-bit data bus) ──────────────────────────────

class Stm8GpioPort : public IGpioPort {
public:
    Stm8GpioPort()
        : m_odr(PB_ODR), m_idr(PB_IDR), m_ddr(PB_DDR), m_cr1(PB_CR1) {}

    void setOutputMask(uint8_t mask) override {
        m_ddr |= mask;
        m_cr1 |= mask;   // push-pull
    }
    void setInputMask(uint8_t mask) override {
        m_ddr &= ~mask;
        m_cr1 &= ~mask;
    }
    void setOpenDrainMask(uint8_t mask) override {
        m_ddr |= mask;
        m_cr1 &= ~mask;  // open-drain
    }
    void write(uint8_t value) override  { m_odr = value; }
    uint8_t read() const override       { return m_idr;  }

private:
    volatile uint8_t& m_odr;
    volatile uint8_t& m_idr;
    volatile uint8_t& m_ddr;
    volatile uint8_t& m_cr1;
};

// ── Stm8Spi ──────────────────────────────────────────────────────────────────

class Stm8Spi : public ISpi {
public:
    void init() override {
        // Enable 16 MHz HSI (default), no prescaler
        CLK_CKDIVR = 0x00;

        // Configure SPI pins (PC5=SCK, PC6=MOSI output; PC7=MISO input)
        PC_DDR |= (1 << 5) | (1 << 6);
        PC_CR1 |= (1 << 5) | (1 << 6);
        PC_DDR &= ~(1 << 7);

        // CS pin PA3
        PA_DDR |= (1 << 3);
        PA_CR1 |= (1 << 3);
        PA_ODR |= (1 << 3);  // CS de-asserted

        // SPI_CR1: SPE=1, MSTR=1, fPCLK/8 (BR=010), CPOL=0, CPHA=0
        SPI_CR1 = 0x34;   // 0b00110100
        SPI_CR2 = 0x00;
    }

    uint8_t transfer(uint8_t data) override {
        while (!(SPI_SR & SPI_SR_TXE)) {}
        SPI_DR = data;
        while (!(SPI_SR & SPI_SR_RXNE)) {}
        return SPI_DR;
    }

    void csAssert()   override { PA_ODR &= ~(1 << 3); }
    void csDeassert() override { PA_ODR |=  (1 << 3); }
};

// ── Stm8Timer ────────────────────────────────────────────────────────────────

class Stm8Timer : public ITimer {
public:
    void delayMs(uint32_t ms) override {
        while (ms--) delayUs(1000);
    }

    void delayUs(uint32_t us) override {
        // TIM2 configured for 1 µs tick at 16 MHz (prescaler = 16 → 1 MHz)
        TIM2_CR1   = 0x00;
        TIM2_PSCR  = 0x04;  // /16 → 1 MHz tick
        TIM2_ARRH  = static_cast<uint8_t>((us >> 8) & 0xFF);
        TIM2_ARRL  = static_cast<uint8_t>(us & 0xFF);
        TIM2_SR1   = 0x00;
        TIM2_EGR   = 0x01;  // generate update (reload prescaler)
        TIM2_CR1   = 0x01;  // enable
        while (!(TIM2_SR1 & TIM2_SR1_UIF)) {}
        TIM2_CR1 = 0x00;
    }

    uint32_t tickMs() const override {
        // Simple 16-bit counter wrap – suitable for short timeouts only
        uint32_t hi = TIM2_CNTRH;
        uint32_t lo = TIM2_CNTRL;
        return (hi << 8) | lo;
    }
};

#endif // __SDCC
