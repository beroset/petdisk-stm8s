// PetDisk Linux simulation entry point.
// Exercises the HAL and drivers in a host environment.
#include "petdisk.hpp"
#include <cstdio>

// Include the Linux HAL implementations
// (They are in a separate TU; we only forward-declare here for main.)
// Forward declarations are not needed – types are referenced via interfaces.

// Minimal stub filesystem for the simulation
class StubFilesystem : public CbmDos::IFilesystem {
public:
    bool open(uint8_t, const char* filename, uint8_t) override {
        std::printf("[FS] open: %s\n", filename);
        return true;
    }
    void close(uint8_t channel) override {
        std::printf("[FS] close channel %u\n", channel);
    }
    size_t read(uint8_t, uint8_t* buf, size_t maxLen) override {
        static const char msg[] = "HELLO FROM PETDISK\r\n";
        static size_t pos = 0;
        size_t avail = sizeof(msg) - 1 - pos;
        if (avail == 0) { pos = 0; return 0; }
        size_t n = avail < maxLen ? avail : maxLen;
        for (size_t i = 0; i < n; ++i) buf[i] = static_cast<uint8_t>(msg[pos + i]);
        pos += n;
        return n;
    }
    size_t write(uint8_t, const uint8_t* buf, size_t len) override {
        std::printf("[FS] write %zu bytes\n", len);
        for (size_t i = 0; i < len; ++i) std::putchar(buf[i]);
        std::putchar('\n');
        return len;
    }
    uint8_t     errorCode()    const override { return 0; }
    const char* errorMessage() const override { return "OK"; }
    void executeCommand(const char* cmd, size_t len) override {
        std::printf("[FS] cmd: %.*s\n", static_cast<int>(len), cmd);
    }
};

int main() {
    std::printf("PetDisk Linux simulation (not connected to hardware)\n");
    std::printf("Use the firmware/tests target to run unit tests.\n");
    return 0;
}
