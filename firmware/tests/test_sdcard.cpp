// SD card driver unit tests.
#include "catch_amalgamated.hpp"
#include "mock_hal.hpp"
#include "sdcard.hpp"
#include <vector>

// In SPI SD protocol, sendCommand() always consumes:
//   1 byte  : "wait-ready" (card returns 0xFF)
//   6 bytes : command frame (card returns 0xFF each)
//   1-8 bytes: R1 polling (we queue R1 at the first poll → 1 retry byte)
// Total per command = 8 bytes (7 don't-care + 1 R1).
// Extra response bytes (R7 for CMD8, OCR for CMD58) follow immediately.

static void queueCmd(MockSpi& spi, uint8_t r1, const std::vector<uint8_t>& extra = {}) {
    for (int i = 0; i < 7; ++i) spi.queueRx(0xFF);  // wait + 6 cmd frame
    spi.queueRx(r1);                                  // R1 on first retry
    for (auto b : extra) spi.queueRx(b);
}

// Build the response stream that a v2 SDHC card produces during init.
static void queueInitSDHC(MockSpi& spi) {
    // 10 power-up clock bytes (CS de-asserted)
    for (int i = 0; i < 10; ++i) spi.queueRx(0xFF);
    // CMD0  → R1 = 0x01 (IDLE)
    queueCmd(spi, 0x01);
    // CMD8  → R1 = 0x01, R7 tail = 0x00 0x00 0x01 0xAA
    queueCmd(spi, 0x01, {0x00, 0x00, 0x01, 0xAA});
    // CMD55 → R1 = 0x01
    queueCmd(spi, 0x01);
    // ACMD41 → R1 = 0x00 (ready on first try)
    queueCmd(spi, 0x00);
    // CMD58 → R1 = 0x00, OCR = 0xC0FF8000 (CCS=1 = SDHC, BUSY=1 = ready)
    queueCmd(spi, 0x00, {0xC0, 0xFF, 0x80, 0x00});
}

// Build init sequence for SD v1 (CMD8 returns ILLEGAL_CMD 0x05).
static void queueInitSDv1(MockSpi& spi) {
    for (int i = 0; i < 10; ++i) spi.queueRx(0xFF);
    // CMD0 → IDLE
    queueCmd(spi, 0x01);
    // CMD8 → illegal command (bit 2 set in R1)
    queueCmd(spi, 0x05);
    // CMD55 → IDLE
    queueCmd(spi, 0x01);
    // ACMD41 → ready
    queueCmd(spi, 0x00);
}

TEST_CASE("SdCard init succeeds for SDHC card", "[sdcard]") {
    MockSpi   spi;
    MockTimer timer;
    queueInitSDHC(spi);

    SdCard sd(spi, timer);
    auto result = sd.init();

    CHECK(result == SdCard::Result::OK);
    CHECK(sd.isHighCapacity() == true);
}

TEST_CASE("SdCard init succeeds for SD v1 card", "[sdcard]") {
    MockSpi   spi;
    MockTimer timer;
    queueInitSDv1(spi);

    SdCard sd(spi, timer);
    auto result = sd.init();

    CHECK(result == SdCard::Result::OK);
    CHECK(sd.isHighCapacity() == false);
}

TEST_CASE("SdCard init returns NotPresent if CMD0 fails", "[sdcard]") {
    MockSpi   spi;
    MockTimer timer;
    // 10 power-up bytes; CMD0 returns all 0xFF (no valid R1 with MSB=0)
    for (int i = 0; i < 10; ++i) spi.queueRx(0xFF);
    // 7 cmd frame + 8 retry bytes, all 0xFF → no valid R1
    for (int i = 0; i < 15; ++i) spi.queueRx(0xFF);

    SdCard sd(spi, timer);
    auto result = sd.init();
    CHECK(result == SdCard::Result::NotPresent);
}

TEST_CASE("SdCard readBlock sends CMD17 and returns data", "[sdcard]") {
    MockSpi   spi;
    MockTimer timer;
    queueInitSDHC(spi);
    SdCard sd(spi, timer);
    REQUIRE(sd.init() == SdCard::Result::OK);
    spi.reset();

    // CMD17: 7 cmd frame + R1=0x00
    queueCmd(spi, 0x00);
    // Some 0xFF before data token, then data token 0xFE
    for (int i = 0; i < 3; ++i) spi.queueRx(0xFF);
    spi.queueRx(0xFE);  // data start token
    // 512 bytes of data
    for (int i = 0; i < 512; ++i) spi.queueRx(static_cast<uint8_t>(i & 0xFF));
    spi.queueRx(0xFF);  // CRC byte 1
    spi.queueRx(0xFF);  // CRC byte 2

    uint8_t buf[512]{};
    auto result = sd.readBlock(0, buf);

    CHECK(result == SdCard::Result::OK);
    for (int i = 0; i < 512; ++i) {
        REQUIRE(buf[i] == static_cast<uint8_t>(i & 0xFF));
    }

    // Verify CMD17 (0x51) was sent
    bool foundCmd17 = false;
    for (auto b : spi.txLog) {
        if (b == 0x51) { foundCmd17 = true; break; }
    }
    CHECK(foundCmd17);
}

TEST_CASE("SdCard readBlock returns Timeout if no data token", "[sdcard]") {
    MockSpi   spi;
    MockTimer timer;
    queueInitSDHC(spi);
    SdCard sd(spi, timer);
    REQUIRE(sd.init() == SdCard::Result::OK);
    spi.reset();

    // CMD17 R1=OK, but no 0xFE data token – all remaining reads return 0xFF
    queueCmd(spi, 0x00);
    // No 0xFE in queue → MockSpi default returns 0xFF → waitToken times out

    uint8_t buf[512]{};
    auto result = sd.readBlock(0, buf);
    CHECK(result == SdCard::Result::Timeout);
}

TEST_CASE("SdCard writeBlock sends CMD24 and data", "[sdcard]") {
    MockSpi   spi;
    MockTimer timer;
    queueInitSDHC(spi);
    SdCard sd(spi, timer);
    REQUIRE(sd.init() == SdCard::Result::OK);
    spi.reset();

    // CMD24 R1 = 0x00
    queueCmd(spi, 0x00);
    // Gap byte before data token (0xFF returned by default)
    spi.queueRx(0xFF);
    // Data start token sent by us – card returns don't-care (0xFF)
    spi.queueRx(0xFF);
    // 512 data bytes – card returns don't-care (0xFF) each
    for (int i = 0; i < 512; ++i) spi.queueRx(0xFF);
    // 2 CRC bytes
    spi.queueRx(0xFF); spi.queueRx(0xFF);
    // Data response token: 0xE5 (lower 5 bits = 0x05 = accepted)
    spi.queueRx(0xE5);
    // Not busy (0xFF = not busy)
    spi.queueRx(0xFF);

    uint8_t buf[512];
    for (int i = 0; i < 512; ++i) buf[i] = static_cast<uint8_t>(i & 0xFF);
    auto result = sd.writeBlock(1, buf);

    CHECK(result == SdCard::Result::OK);

    // CMD24 = 0x58
    bool foundCmd24 = false;
    for (auto b : spi.txLog) {
        if (b == 0x58) { foundCmd24 = true; break; }
    }
    CHECK(foundCmd24);
}

TEST_CASE("SdCard SDHC uses block addressing in CMD17", "[sdcard]") {
    MockSpi   spi;
    MockTimer timer;
    queueInitSDHC(spi);
    SdCard sd(spi, timer);
    REQUIRE(sd.init() == SdCard::Result::OK);
    REQUIRE(sd.isHighCapacity());
    spi.reset();

    queueCmd(spi, 0x00);
    for (int i = 0; i < 3; ++i) spi.queueRx(0xFF);
    spi.queueRx(0xFE);
    for (int i = 0; i < 512; ++i) spi.queueRx(0x00);
    spi.queueRx(0xFF); spi.queueRx(0xFF);

    uint8_t buf[512]{};
    sd.readBlock(5, buf);

    // Find CMD17 (0x51) in tx log and check address bytes = 0x00000005
    for (size_t i = 0; i + 4 < spi.txLog.size(); ++i) {
        if (spi.txLog[i] == 0x51) {
            uint32_t addr = (static_cast<uint32_t>(spi.txLog[i+1]) << 24) |
                            (static_cast<uint32_t>(spi.txLog[i+2]) << 16) |
                            (static_cast<uint32_t>(spi.txLog[i+3]) << 8)  |
                             static_cast<uint32_t>(spi.txLog[i+4]);
            CHECK(addr == 5);  // SDHC: block number 5, not byte 5×512
            break;
        }
    }
}

