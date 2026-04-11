#pragma once
// SPI-mode SD card driver for PetDisk.
// Supports SD v1, v2 and SDHC; CRC disabled for speed.
#include "hal.hpp"
#include <cstddef>
#include <cstdint>

class SdCard {
public:
    enum class Result { OK, Error, Timeout, NotPresent };

    static constexpr size_t kBlockSize = 512;

    explicit SdCard(ISpi& spi, ITimer& timer);

    // Run the SD initialisation sequence.
    // Must be called before readBlock / writeBlock.
    Result init();

    // Read one 512-byte block.
    Result readBlock(uint32_t blockNum, uint8_t* buffer);

    // Write one 512-byte block.
    Result writeBlock(uint32_t blockNum, const uint8_t* buffer);

    // True if card is SDHC/SDXC (block addressing), false if byte addressing.
    bool isHighCapacity() const { return m_highCapacity; }

    // Returns the OCR value read during init (useful for diagnostics).
    uint32_t ocr() const { return m_ocr; }

private:
    // Low-level SPI helpers
    uint8_t  sendCommand(uint8_t cmd, uint32_t arg);
    uint8_t  sendAcmd(uint8_t cmd, uint32_t arg);
    uint8_t  waitReady(uint32_t timeoutLoops = 500'000u);
    bool     waitToken(uint8_t token, uint32_t timeoutLoops = 500'000u);

    ISpi&   m_spi;
    ITimer& m_timer;
    bool    m_highCapacity{false};
    uint32_t m_ocr{0};
};
