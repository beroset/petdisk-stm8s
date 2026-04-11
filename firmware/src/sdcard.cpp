// SPI-mode SD card driver for PetDisk.
// Follows the simplified SPI SD card protocol:
//   - All commands are 6 bytes (start, cmd, arg[4], CRC)
//   - Responses: R1 (1 byte), R3/R7 (5 bytes)
//   - CRC is disabled after init except for CMD0/CMD8
#include "sdcard.hpp"
#include <cstring>

// SD SPI command bytes
static constexpr uint8_t CMD0   = 0x40;  // GO_IDLE_STATE
static constexpr uint8_t CMD8   = 0x48;  // SEND_IF_COND
static constexpr uint8_t CMD17  = 0x51;  // READ_SINGLE_BLOCK
static constexpr uint8_t CMD24  = 0x58;  // WRITE_BLOCK
static constexpr uint8_t CMD55  = 0x77;  // APP_CMD (prefix for ACMD)
static constexpr uint8_t CMD58  = 0x7A;  // READ_OCR
static constexpr uint8_t ACMD41 = 0x69; // SD_SEND_OP_COND

// R1 status bits
static constexpr uint8_t R1_IDLE        = 0x01;
static constexpr uint8_t R1_ILLEGAL_CMD = 0x04;

// Data tokens
static constexpr uint8_t TOKEN_DATA_START        = 0xFE;
static constexpr uint8_t TOKEN_DATA_ACCEPT       = 0x05;
static constexpr uint8_t TOKEN_DATA_ACCEPT_MASK  = 0x1F;

// OCR bit for CCS (card capacity status)
static constexpr uint32_t OCR_CCS = (1u << 30);
static constexpr uint32_t OCR_BUSY = (1u << 31); // 1 = ready

SdCard::SdCard(ISpi& spi, ITimer& timer)
    : m_spi(spi), m_timer(timer)
{}

// Send 0xFF bytes until the card returns a non-0xFF byte (card is not busy).
uint8_t SdCard::waitReady(uint32_t timeoutLoops) {
    uint8_t r;
    do {
        r = m_spi.transfer(0xFF);
    } while (r == 0xFF && --timeoutLoops);
    return r;
}

// Wait until the card sends a specific token byte.
bool SdCard::waitToken(uint8_t token, uint32_t timeoutLoops) {
    uint8_t r;
    do {
        r = m_spi.transfer(0xFF);
        if (r == token) return true;
    } while (--timeoutLoops);
    return false;
}

// Send a 6-byte SPI SD command and return the R1 response byte.
// CRC byte is provided pre-computed (only needed for CMD0 and CMD8).
uint8_t SdCard::sendCommand(uint8_t cmd, uint32_t arg) {
    // Pre-computed CRC for CMD0 (0x95) and CMD8 with 0x1AA arg (0x87)
    uint8_t crc = 0xFF;
    if (cmd == CMD0)  crc = 0x95;
    if (cmd == CMD8)  crc = 0x87;

    // Wait until not busy
    m_spi.transfer(0xFF);

    m_spi.transfer(cmd);
    m_spi.transfer(static_cast<uint8_t>(arg >> 24));
    m_spi.transfer(static_cast<uint8_t>(arg >> 16));
    m_spi.transfer(static_cast<uint8_t>(arg >> 8));
    m_spi.transfer(static_cast<uint8_t>(arg));
    m_spi.transfer(crc);

    // Wait for R1 (up to 8 retries – SD spec §7.5.1)
    uint8_t r1 = 0xFF;
    for (int i = 0; i < 8; ++i) {
        r1 = m_spi.transfer(0xFF);
        if (!(r1 & 0x80)) break;
    }
    return r1;
}

// Send an application-specific command (preceded by CMD55).
uint8_t SdCard::sendAcmd(uint8_t cmd, uint32_t arg) {
    sendCommand(CMD55, 0);
    return sendCommand(cmd, arg);
}

SdCard::Result SdCard::init() {
    m_spi.init();
    m_highCapacity = false;
    m_ocr = 0;

    // SD card requires ≥74 clock cycles with CS de-asserted before any command
    m_spi.csDeassert();
    for (int i = 0; i < 10; ++i) {
        m_spi.transfer(0xFF);
    }

    m_spi.csAssert();

    // CMD0: reset into SPI mode (expect R1 = 0x01 IDLE)
    uint8_t r1 = sendCommand(CMD0, 0);
    if (r1 != R1_IDLE) {
        m_spi.csDeassert();
        return Result::NotPresent;
    }

    // CMD8: check for SD v2 (voltage range 2.7-3.6V, check pattern 0xAA)
    bool isV2 = false;
    r1 = sendCommand(CMD8, 0x000001AA);
    if (!(r1 & R1_ILLEGAL_CMD)) {
        // SD v2: read 4-byte R7 tail
        uint8_t r7[4];
        for (auto& b : r7) b = m_spi.transfer(0xFF);
        // Check echo-back pattern and voltage
        if (r7[2] == 0x01 && r7[3] == 0xAA) {
            isV2 = true;
        }
    }

    // ACMD41: initialise card (with HCS bit set for v2)
    uint32_t acmd41arg = isV2 ? 0x40000000u : 0u;
    uint32_t retries = 1000;
    do {
        r1 = sendAcmd(ACMD41, acmd41arg);
        if (r1 == 0) break;
        m_timer.delayMs(1);
    } while (--retries);

    if (r1 != 0) {
        m_spi.csDeassert();
        return Result::Timeout;
    }

    // CMD58: read OCR to determine SDHC vs SDSC
    if (isV2) {
        r1 = sendCommand(CMD58, 0);
        if (r1 == 0) {
            m_ocr  = static_cast<uint32_t>(m_spi.transfer(0xFF)) << 24;
            m_ocr |= static_cast<uint32_t>(m_spi.transfer(0xFF)) << 16;
            m_ocr |= static_cast<uint32_t>(m_spi.transfer(0xFF)) << 8;
            m_ocr |= m_spi.transfer(0xFF);
            m_highCapacity = (m_ocr & OCR_CCS) != 0;
        }
    }

    m_spi.csDeassert();
    m_spi.transfer(0xFF);  // extra clock to finalise
    return Result::OK;
}

SdCard::Result SdCard::readBlock(uint32_t blockNum, uint8_t* buffer) {
    // For SDSC cards addresses are in bytes; SDHC uses block numbers
    uint32_t addr = m_highCapacity ? blockNum : blockNum * kBlockSize;

    m_spi.csAssert();
    uint8_t r1 = sendCommand(CMD17, addr);
    if (r1 != 0) {
        m_spi.csDeassert();
        return Result::Error;
    }

    // Wait for data start token (0xFE)
    if (!waitToken(TOKEN_DATA_START)) {
        m_spi.csDeassert();
        return Result::Timeout;
    }

    for (size_t i = 0; i < kBlockSize; ++i) {
        buffer[i] = m_spi.transfer(0xFF);
    }
    // Discard 2 CRC bytes
    m_spi.transfer(0xFF);
    m_spi.transfer(0xFF);

    m_spi.csDeassert();
    m_spi.transfer(0xFF);
    return Result::OK;
}

SdCard::Result SdCard::writeBlock(uint32_t blockNum, const uint8_t* buffer) {
    uint32_t addr = m_highCapacity ? blockNum : blockNum * kBlockSize;

    m_spi.csAssert();
    uint8_t r1 = sendCommand(CMD24, addr);
    if (r1 != 0) {
        m_spi.csDeassert();
        return Result::Error;
    }

    // Short gap before data
    m_spi.transfer(0xFF);

    // Send data start token
    m_spi.transfer(TOKEN_DATA_START);

    for (size_t i = 0; i < kBlockSize; ++i) {
        m_spi.transfer(buffer[i]);
    }
    // Dummy CRC
    m_spi.transfer(0xFF);
    m_spi.transfer(0xFF);

    // Read data response token
    uint8_t resp = m_spi.transfer(0xFF);
    if ((resp & TOKEN_DATA_ACCEPT_MASK) != TOKEN_DATA_ACCEPT) {
        m_spi.csDeassert();
        return Result::Error;
    }

    // Wait for write to complete (busy = 0x00)
    uint32_t timeout = 500'000u;
    while (m_spi.transfer(0xFF) == 0x00) {
        if (--timeout == 0) {
            m_spi.csDeassert();
            return Result::Timeout;
        }
    }

    m_spi.csDeassert();
    m_spi.transfer(0xFF);
    return Result::OK;
}
