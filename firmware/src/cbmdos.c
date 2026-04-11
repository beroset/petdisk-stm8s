/* Commodore DOS protocol handler for PetDisk.
 * Implements the CBM disk drive protocol on top of the IEEE-488 driver.
 */
#include "cbmdos.h"
#include <string.h>
#include <stdio.h>

/* Open-file modes */
#define MODE_READ  0u
#define MODE_WRITE 1u

/* CBM error codes */
#define ERR_OK             0u
#define ERR_FILE_NOT_FOUND 62u
#define ERR_SYNTAX         31u
#define ERR_WRITE_PROTECT  26u

/* Convenience macros for IFilesystem vtable calls */
#define fs_open(f, ch, fn, m) (f)->open(f, ch, fn, m)
#define fs_close(f, ch)       (f)->close(f, ch)
#define fs_read(f, ch, b, n)  (f)->read(f, ch, b, n)
#define fs_write(f, ch, b, n) (f)->write(f, ch, b, n)
#define fs_errorCode(f)       (f)->errorCode(f)
#define fs_errorMessage(f)    (f)->errorMessage(f)
#define fs_execCmd(f, c, l)   (f)->executeCommand(f, c, l)

static void buildStatusString(CbmDos* self);

void CbmDos_init(CbmDos* self, Ieee488* gpib, IFilesystem* fs)
{
    memset(self, 0, sizeof(*self));
    self->gpib = gpib;
    self->fs   = fs;
    CbmDos_reset(self);
}

void CbmDos_reset(CbmDos* self)
{
    uint8_t i;
    for (i = 0; i < CBMDOS_NUM_CHANNELS; ++i) {
        self->channels[i].open      = 0;
        self->channels[i].mode      = 0;
        self->channels[i].filename[0] = '\0';
    }
    self->errorCode   = ERR_OK;
    self->errorTrack  = 0;
    self->errorSector = 0;
    buildStatusString(self);
}

void CbmDos_setError(CbmDos* self, uint8_t code, const char* message,
                     uint8_t track, uint8_t sector)
{
    self->errorCode   = code;
    self->errorTrack  = track;
    self->errorSector = sector;
    snprintf(self->statusBuf, sizeof(self->statusBuf),
             "%02u, %s,%02u,%02u\r", (unsigned)code, message,
             (unsigned)track, (unsigned)sector);
}

static void buildStatusString(CbmDos* self)
{
    CbmDos_setError(self, self->errorCode,
                    self->errorCode == ERR_OK ? "OK" : "ERROR",
                    self->errorTrack, self->errorSector);
}

const char* CbmDos_getStatus(const CbmDos* self)
{
    return self->statusBuf;
}

void CbmDos_processCommand(CbmDos* self, uint8_t channel,
                            const char* cmd, size_t len)
{
    if (len == 0) return;

    if (channel == CBMDOS_CMD_CHANNEL) {
        fs_execCmd(self->fs, cmd, len);
        if (fs_errorCode(self->fs) != 0) {
            CbmDos_setError(self, fs_errorCode(self->fs),
                            fs_errorMessage(self->fs), 0, 0);
        } else {
            CbmDos_setError(self, ERR_OK, "OK", 0, 0);
        }
        return;
    }

    if (channel < CBMDOS_NUM_CHANNELS) {
        CbmDosChannel* ch = &self->channels[channel];
        size_t copy = len < sizeof(ch->filename) - 1
                          ? len
                          : sizeof(ch->filename) - 1;
        memcpy(ch->filename, cmd, copy);
        ch->filename[copy] = '\0';

        ch->mode = (channel == 1) ? MODE_WRITE : MODE_READ;

        if (fs_open(self->fs, channel, ch->filename, ch->mode)) {
            ch->open = 1;
            CbmDos_setError(self, ERR_OK, "OK", 0, 0);
        } else {
            ch->open = 0;
            if (fs_errorCode(self->fs) != 0) {
                CbmDos_setError(self, fs_errorCode(self->fs),
                                fs_errorMessage(self->fs), 0, 0);
            } else {
                CbmDos_setError(self, ERR_FILE_NOT_FOUND,
                                "FILE NOT FOUND", 0, 0);
            }
        }
    }
}

void CbmDos_closeChannel(CbmDos* self, uint8_t channel)
{
    if (channel >= CBMDOS_NUM_CHANNELS) return;
    if (self->channels[channel].open) {
        fs_close(self->fs, channel);
        self->channels[channel].open = 0;
    }
}

size_t CbmDos_readChannel(CbmDos* self, uint8_t channel,
                           uint8_t* buf, size_t maxLen)
{
    if (channel == CBMDOS_CMD_CHANNEL) {
        const char* s = self->statusBuf;
        size_t slen = strlen(s);
        size_t n = slen < maxLen ? slen : maxLen;
        memcpy(buf, s, n);
        /* After reading the status, reset to OK */
        CbmDos_setError(self, ERR_OK, "OK", 0, 0);
        return n;
    }
    if (channel >= CBMDOS_NUM_CHANNELS || !self->channels[channel].open) return 0;
    return fs_read(self->fs, channel, buf, maxLen);
}

size_t CbmDos_writeChannel(CbmDos* self, uint8_t channel,
                            const uint8_t* buf, size_t len)
{
    if (channel == CBMDOS_CMD_CHANNEL) {
        CbmDos_processCommand(self, channel, (const char*)buf, len);
        return len;
    }
    if (channel >= CBMDOS_NUM_CHANNELS || !self->channels[channel].open) return 0;
    return fs_write(self->fs, channel, buf, len);
}
