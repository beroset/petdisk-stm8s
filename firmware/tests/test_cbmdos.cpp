// CBM DOS protocol handler unit tests.
#include "catch_amalgamated.hpp"
#include "mock_hal.hpp"
#include "cbmdos.hpp"
#include <cstring>

// We need a minimal Ieee488 instance even though CbmDos tests don't exercise it.
struct CbmFixture {
    MockGpioPort dataBus;
    MockGpioPin  dav, nrfd, ndac, atn, eoi, srq, ifc, ren;
    Ieee488      ieee{dataBus, dav, nrfd, ndac, atn, eoi, srq, ifc, ren, 8};
    MockFilesystem fs;
    CbmDos       dos{ieee, fs};

    CbmFixture() {
        atn.inject(true);
        ifc.inject(true);
        ren.inject(true);
        dav.inject(true);
        nrfd.inject(true);
        ndac.inject(true);
        ieee.init();
        dos.reset();
    }
};

// ── Status channel ─────────────────────────────────────────────────────────

TEST_CASE("CbmDos initial status is OK", "[cbmdos]") {
    CbmFixture f;
    const char* s = f.dos.getStatus();
    CHECK(std::strncmp(s, "00, OK", 6) == 0);
}

TEST_CASE("CbmDos setError stores correct status string", "[cbmdos]") {
    CbmFixture f;
    f.dos.setError(62, "FILE NOT FOUND", 0, 0);
    const char* s = f.dos.getStatus();
    // Should start with "62, FILE NOT FOUND"
    CHECK(std::strncmp(s, "62, FILE NOT FOUND", 18) == 0);
}

TEST_CASE("CbmDos readChannel 15 returns status string", "[cbmdos]") {
    CbmFixture f;
    f.dos.setError(0, "OK", 0, 0);

    uint8_t buf[64]{};
    size_t n = f.dos.readChannel(15, buf, sizeof(buf));

    REQUIRE(n > 0);
    buf[n] = '\0';
    CHECK(std::strncmp(reinterpret_cast<char*>(buf), "00, OK", 6) == 0);
}

TEST_CASE("CbmDos readChannel 15 resets status to OK after read", "[cbmdos]") {
    CbmFixture f;
    f.dos.setError(62, "FILE NOT FOUND", 0, 0);

    uint8_t buf[64]{};
    f.dos.readChannel(15, buf, sizeof(buf));

    // After reading, status should reset to OK
    const char* s = f.dos.getStatus();
    CHECK(std::strncmp(s, "00, OK", 6) == 0);
}

// ── Channel open / close ───────────────────────────────────────────────────

TEST_CASE("CbmDos processCommand opens data channel", "[cbmdos]") {
    CbmFixture f;
    f.fs.openResult = true;

    f.dos.processCommand(2, "DATA.PRG", 8);

    REQUIRE(f.fs.openCalls.size() == 1);
    CHECK(f.fs.openCalls[0].channel == 2);
    CHECK(f.fs.openCalls[0].filename == "DATA.PRG");
    CHECK(f.fs.openCalls[0].mode == 0);  // channel 2 = read
}

TEST_CASE("CbmDos processCommand channel 1 opens in write mode", "[cbmdos]") {
    CbmFixture f;
    f.dos.processCommand(1, "OUT.SEQ", 7);

    REQUIRE(f.fs.openCalls.size() == 1);
    CHECK(f.fs.openCalls[0].mode == 1);  // channel 1 = write
}

TEST_CASE("CbmDos sets FILE NOT FOUND when open fails", "[cbmdos]") {
    CbmFixture f;
    f.fs.openResult    = false;
    f.fs.lastErrorCode = 62;
    f.fs.lastErrorMsg  = "FILE NOT FOUND";

    f.dos.processCommand(2, "MISSING", 7);

    const char* s = f.dos.getStatus();
    CHECK(std::strncmp(s, "62,", 3) == 0);
}

TEST_CASE("CbmDos closeChannel calls filesystem close", "[cbmdos]") {
    CbmFixture f;
    f.dos.processCommand(3, "TEST.PRG", 8);  // open it first
    f.dos.closeChannel(3);

    REQUIRE(f.fs.closedChannels.size() == 1);
    CHECK(f.fs.closedChannels[0] == 3);
}

TEST_CASE("CbmDos closeChannel on unopened channel is harmless", "[cbmdos]") {
    CbmFixture f;
    CHECK_NOTHROW(f.dos.closeChannel(5));
    CHECK(f.fs.closedChannels.empty());
}

// ── Read / write data channels ─────────────────────────────────────────────

TEST_CASE("CbmDos readChannel returns filesystem data", "[cbmdos]") {
    CbmFixture f;
    f.dos.processCommand(2, "FILE.PRG", 8);
    f.fs.readData = {0x01, 0x02, 0x03, 0x04};

    uint8_t buf[8]{};
    size_t n = f.dos.readChannel(2, buf, sizeof(buf));

    CHECK(n == 4);
    CHECK(buf[0] == 0x01);
    CHECK(buf[3] == 0x04);
}

TEST_CASE("CbmDos readChannel on closed channel returns 0", "[cbmdos]") {
    CbmFixture f;
    uint8_t buf[8]{};
    size_t n = f.dos.readChannel(4, buf, sizeof(buf));
    CHECK(n == 0);
}

TEST_CASE("CbmDos writeChannel passes data to filesystem", "[cbmdos]") {
    CbmFixture f;
    f.dos.processCommand(1, "SAVE.SEQ", 8);

    const uint8_t data[] = {0xAA, 0xBB, 0xCC};
    size_t n = f.dos.writeChannel(1, data, sizeof(data));

    CHECK(n == 3);
    REQUIRE(f.fs.writtenData.size() == 3);
    CHECK(f.fs.writtenData[0] == 0xAA);
    CHECK(f.fs.writtenData[2] == 0xCC);
}

TEST_CASE("CbmDos writeChannel to ch 15 executes command", "[cbmdos]") {
    CbmFixture f;
    const char cmd[] = "S:OLD.PRG";
    f.dos.writeChannel(15, reinterpret_cast<const uint8_t*>(cmd), sizeof(cmd) - 1);

    CHECK(f.fs.lastCommand == "S:OLD.PRG");
}

// ── Command channel passthrough ─────────────────────────────────────────────

TEST_CASE("CbmDos processCommand on ch 15 executes filesystem command", "[cbmdos]") {
    CbmFixture f;
    f.dos.processCommand(15, "I", 1);  // initialize
    CHECK(f.fs.lastCommand == "I");
}

TEST_CASE("CbmDos reset clears all channels", "[cbmdos]") {
    CbmFixture f;
    f.dos.processCommand(2, "FILE.PRG", 8);
    f.dos.reset();

    // After reset, channel 2 should be closed
    uint8_t buf[8]{};
    size_t n = f.dos.readChannel(2, buf, sizeof(buf));
    CHECK(n == 0);
}

// ── Status string format ───────────────────────────────────────────────────

TEST_CASE("CbmDos status string has track and sector fields", "[cbmdos]") {
    CbmFixture f;
    f.dos.setError(21, "READ ERROR", 18, 3);
    const char* s = f.dos.getStatus();
    // Expected: "21, READ ERROR,18,03\r"
    CHECK(std::strstr(s, "18") != nullptr);
    CHECK(std::strstr(s, "03") != nullptr);
}

TEST_CASE("CbmDos status string ends with carriage return", "[cbmdos]") {
    CbmFixture f;
    const char* s = f.dos.getStatus();
    size_t len = std::strlen(s);
    REQUIRE(len > 0);
    CHECK(s[len - 1] == '\r');
}
