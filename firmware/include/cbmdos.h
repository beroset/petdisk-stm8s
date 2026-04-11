#pragma once
/* Commodore DOS protocol handler for PetDisk.
 * Sits on top of Ieee488 and implements the CBM disk drive protocol:
 *   - Channels 0-14: file data
 *   - Channel 15:    command / status
 */
#include "ieee488.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CBMDOS_NUM_CHANNELS 16u
#define CBMDOS_CMD_CHANNEL  15u

/* ── Per-channel state ───────────────────────────────────────────────────── */
typedef struct {
    uint8_t open;
    uint8_t mode;     /* 0=read, 1=write */
    char    filename[64];
} CbmDosChannel;

/* ── CbmDos state ────────────────────────────────────────────────────────── */
typedef struct {
    Ieee488*    gpib;
    IFilesystem* fs;
    CbmDosChannel channels[CBMDOS_NUM_CHANNELS];
    uint8_t     errorCode;
    uint8_t     errorTrack;
    uint8_t     errorSector;
    char        statusBuf[64];
} CbmDos;

/* Initialise the CbmDos struct and reset all channels to closed. */
void CbmDos_init(CbmDos* self, Ieee488* gpib, IFilesystem* fs);

/* Process a command received on channel 15. */
void CbmDos_processCommand(CbmDos* self, uint8_t channel,
                            const char* cmd, size_t len);

/* Close channel. */
void CbmDos_closeChannel(CbmDos* self, uint8_t channel);

/* Read data from a channel (PET is reading from us as talker).
 * Returns number of bytes placed in buf. */
size_t CbmDos_readChannel(CbmDos* self, uint8_t channel,
                           uint8_t* buf, size_t maxLen);

/* Write data to a channel (PET is writing to us as listener).
 * Returns number of bytes accepted. */
size_t CbmDos_writeChannel(CbmDos* self, uint8_t channel,
                            const uint8_t* buf, size_t len);

/* Return the status / error string for channel 15 (e.g. "00, OK,00,00\r"). */
const char* CbmDos_getStatus(const CbmDos* self);

/* Set the error status explicitly (used internally and in tests). */
void CbmDos_setError(CbmDos* self, uint8_t code, const char* message,
                     uint8_t track, uint8_t sector);

/* Reset all channels and clear error status. */
void CbmDos_reset(CbmDos* self);

#ifdef __cplusplus
} /* extern "C" */
#endif
