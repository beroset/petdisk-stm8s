// Commodore DOS protocol handler for PetDisk.
// Implements the CBM disk drive protocol on top of the IEEE-488 driver.
#include "cbmdos.hpp"
#include <cstring>
#include <cstdio>

// Open-file modes
static constexpr uint8_t MODE_READ  = 0;
static constexpr uint8_t MODE_WRITE = 1;

// CBM error codes
static constexpr uint8_t ERR_OK              = 0;
static constexpr uint8_t ERR_FILE_NOT_FOUND  = 62;
static constexpr uint8_t ERR_SYNTAX          = 31;
static constexpr uint8_t ERR_WRITE_PROTECT   = 26;

CbmDos::CbmDos(Ieee488& gpib, IFilesystem& fs)
    : m_gpib(gpib), m_fs(fs)
{
    reset();
}

void CbmDos::reset() {
    for (auto& ch : m_channels) {
        ch.open = false;
        ch.mode = 0;
        ch.filename[0] = '\0';
    }
    m_errorCode   = ERR_OK;
    m_errorTrack  = 0;
    m_errorSector = 0;
    buildStatusString();
}

void CbmDos::setError(uint8_t code, const char* message, uint8_t track, uint8_t sector) {
    m_errorCode   = code;
    m_errorTrack  = track;
    m_errorSector = sector;
    // Build status string: "CC, MESSAGE,TT,SS\r"
    // (We ignore message param for the standard format; it is encoded in code.)
    // Store the message directly so tests can inject arbitrary text.
    std::snprintf(m_statusBuf, sizeof(m_statusBuf),
                  "%02u, %s,%02u,%02u\r", code, message, track, sector);
}

void CbmDos::buildStatusString() {
    setError(m_errorCode,
             m_errorCode == ERR_OK ? "OK" : "ERROR",
             m_errorTrack, m_errorSector);
}

const char* CbmDos::getStatus() const {
    return m_statusBuf;
}

// Process a command received on channel 15 (command channel).
// Commands follow CBM DOS conventions:
//   OPEN ch,8,15,"I" – initialize
//   OPEN ch,8,15,"S:file" – scratch
//   OPEN ch,8,15,"R:new=old" – rename
//   For data channels, OPEN ch,8,n,"filename" opens a file.
void CbmDos::processCommand(uint8_t channel, const char* cmd, size_t len) {
    if (len == 0) return;

    if (channel == CMD_CHANNEL) {
        // Commands on channel 15
        m_fs.executeCommand(cmd, len);
        if (m_fs.errorCode() != 0) {
            setError(m_fs.errorCode(), m_fs.errorMessage());
        } else {
            setError(ERR_OK, "OK");
        }
        return;
    }

    // Data channel OPEN: the command is the filename (with optional type suffix)
    if (channel < NUM_CHANNELS) {
        Channel& ch = m_channels[channel];
        // Copy filename (truncate to fit)
        size_t copy = len < sizeof(ch.filename) - 1 ? len : sizeof(ch.filename) - 1;
        std::memcpy(ch.filename, cmd, copy);
        ch.filename[copy] = '\0';

        // Determine mode from channel number: 0 = read, 1 = write, 2+ = read
        ch.mode = (channel == 1) ? MODE_WRITE : MODE_READ;

        bool ok = m_fs.open(channel, ch.filename, ch.mode);
        if (ok) {
            ch.open = true;
            setError(ERR_OK, "OK");
        } else {
            ch.open = false;
            setError(m_fs.errorCode() != 0 ? m_fs.errorCode() : ERR_FILE_NOT_FOUND,
                     m_fs.errorCode() != 0 ? m_fs.errorMessage() : "FILE NOT FOUND");
        }
    }
}

void CbmDos::closeChannel(uint8_t channel) {
    if (channel >= NUM_CHANNELS) return;
    if (m_channels[channel].open) {
        m_fs.close(channel);
        m_channels[channel].open = false;
    }
}

// Read data from a channel (PET is reading from us as talker).
size_t CbmDos::readChannel(uint8_t channel, uint8_t* buf, size_t maxLen) {
    if (channel == CMD_CHANNEL) {
        // Return status string
        const char* s = m_statusBuf;
        size_t slen = std::strlen(s);
        size_t n = slen < maxLen ? slen : maxLen;
        std::memcpy(buf, s, n);
        // After reading the status, reset to OK
        setError(ERR_OK, "OK");
        return n;
    }
    if (channel >= NUM_CHANNELS || !m_channels[channel].open) return 0;
    return m_fs.read(channel, buf, maxLen);
}

// Write data to a channel (PET is writing to us as listener).
size_t CbmDos::writeChannel(uint8_t channel, const uint8_t* buf, size_t len) {
    if (channel == CMD_CHANNEL) {
        // Command sent to channel 15
        processCommand(channel, reinterpret_cast<const char*>(buf), len);
        return len;
    }
    if (channel >= NUM_CHANNELS || !m_channels[channel].open) return 0;
    return m_fs.write(channel, buf, len);
}
