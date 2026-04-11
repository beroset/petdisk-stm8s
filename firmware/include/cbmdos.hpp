#pragma once
// Commodore DOS protocol handler for PetDisk.
// Sits on top of Ieee488 and implements the CBM disk drive protocol:
//   - Channels 0-14: file data
//   - Channel 15:    command / status
#include "ieee488.hpp"
#include <cstdint>
#include <cstddef>

class CbmDos {
public:
    static constexpr uint8_t NUM_CHANNELS = 16;
    static constexpr uint8_t CMD_CHANNEL  = 15;

    // Filesystem callback interface – keeps CbmDos testable without a real FS.
    struct IFilesystem {
        virtual ~IFilesystem() = default;

        // Open a file; channel 0 = sequential read, channel 1 = sequential write.
        // Returns true on success.  Error code / message should be set via the
        // implementation-specific mechanism and then queried with errorCode().
        virtual bool open(uint8_t channel, const char* filename, uint8_t mode) = 0;
        virtual void close(uint8_t channel) = 0;

        // Read up to maxLen bytes into buf; returns number of bytes read (0 = EOF).
        virtual size_t read(uint8_t channel, uint8_t* buf, size_t maxLen) = 0;

        // Write len bytes from buf; returns number of bytes written.
        virtual size_t write(uint8_t channel, const uint8_t* buf, size_t len) = 0;

        // CBM error code for last error (0 = no error).
        virtual uint8_t errorCode() const = 0;

        // Human-readable error message for last error.
        virtual const char* errorMessage() const = 0;

        // Execute a command string (e.g. "S:file", "R:old=new").
        virtual void executeCommand(const char* cmd, size_t len) = 0;
    };

    CbmDos(Ieee488& gpib, IFilesystem& fs);

    // Process a command received on channel 15.
    void processCommand(uint8_t channel, const char* cmd, size_t len);

    // Close channel.
    void closeChannel(uint8_t channel);

    // Read data from a channel (called when PET is reading from us as talker).
    size_t readChannel(uint8_t channel, uint8_t* buf, size_t maxLen);

    // Write data to a channel (called when PET is writing to us as listener).
    size_t writeChannel(uint8_t channel, const uint8_t* buf, size_t len);

    // Return the status / error string for channel 15 (e.g. "00, OK,00,00\r").
    const char* getStatus() const;

    // Set the error status explicitly (used internally and in tests).
    void setError(uint8_t code, const char* message, uint8_t track = 0, uint8_t sector = 0);

    // Reset all channels and clear error status.
    void reset();

private:
    void buildStatusString();

    Ieee488&     m_gpib;
    IFilesystem& m_fs;

    struct Channel {
        bool    open{false};
        uint8_t mode{0};   // 0=read, 1=write
        char    filename[64]{};
    };
    Channel m_channels[NUM_CHANNELS]{};

    // Status / error state
    uint8_t m_errorCode{0};
    uint8_t m_errorTrack{0};
    uint8_t m_errorSector{0};
    char    m_statusBuf[64]{};
};
