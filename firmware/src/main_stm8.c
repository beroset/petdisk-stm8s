/* PetDisk STM8 entry point.
 * Initialises the HAL, wires up the application, and runs the main loop.
 *
 * A minimal stub IFilesystem is provided here.  Replace it with a real FAT
 * filesystem implementation once one is available.
 */
#include "petdisk.h"
#include "hal_stm8.h"
#include <stddef.h>
#include <string.h>

/* ── Stub filesystem (returns FILE NOT FOUND for every operation) ────────── */

static int stub_open(IFilesystem* self, uint8_t channel,
                     const char* filename, uint8_t mode)
{
    (void)self; (void)channel; (void)filename; (void)mode;
    return 0;   /* failure – file not found */
}

static void stub_close(IFilesystem* self, uint8_t channel)
{
    (void)self; (void)channel;
}

static size_t stub_read(IFilesystem* self, uint8_t channel,
                        uint8_t* buf, size_t maxLen)
{
    (void)self; (void)channel; (void)buf; (void)maxLen;
    return 0;
}

static size_t stub_write(IFilesystem* self, uint8_t channel,
                         const uint8_t* buf, size_t len)
{
    (void)self; (void)channel; (void)buf; (void)len;
    return 0;
}

static uint8_t stub_errorCode(const IFilesystem* self)
{
    (void)self;
    return 62u;   /* FILE NOT FOUND */
}

static const char* stub_errorMessage(const IFilesystem* self)
{
    (void)self;
    return "FILE NOT FOUND";
}

static void stub_executeCommand(IFilesystem* self,
                                const char* cmd, size_t len)
{
    (void)self; (void)cmd; (void)len;
}

static IFilesystem g_stubFs = {
    stub_open,
    stub_close,
    stub_read,
    stub_write,
    stub_errorCode,
    stub_errorMessage,
    stub_executeCommand
};

/* ── Application ─────────────────────────────────────────────────────────── */

static PetDisk g_app;

void main(void)
{
    IGpioPort* dataPort;
    IGpioPin*  dav, *nrfd, *ndac;
    IGpioPin*  atn, *eoi,  *srq;
    IGpioPin*  ifc, *ren;
    ISpi*      spi;
    ITimer*    timer;

    stm8_hal_init(&dataPort,
                  &dav, &nrfd, &ndac,
                  &atn, &eoi,  &srq,
                  &ifc, &ren,
                  &spi, &timer);

    PetDisk_init(&g_app,
                 dataPort,
                 dav, nrfd, ndac,
                 atn, eoi, srq,
                 ifc, ren,
                 spi, timer,
                 &g_stubFs,
                 8u);   /* device address 8 */

    PetDisk_begin(&g_app);

    for (;;) {
        PetDisk_run(&g_app);
    }
}
