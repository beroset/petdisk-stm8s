/* SPI-mode SD card driver for PetDisk.
 * Follows the simplified SPI SD card protocol:
 *   - All commands are 6 bytes (start, cmd, arg[4], CRC)
 *   - Responses: R1 (1 byte), R3/R7 (5 bytes)
 *   - CRC is disabled after init except for CMD0/CMD8
 */
#include "sdcard.h"
#include <string.h>

/* SD SPI command bytes */
#define CMD0   0x40u   /* GO_IDLE_STATE     */
#define CMD8   0x48u   /* SEND_IF_COND      */
#define CMD17  0x51u   /* READ_SINGLE_BLOCK */
#define CMD24  0x58u   /* WRITE_BLOCK       */
#define CMD55  0x77u   /* APP_CMD (ACMD prefix) */
#define CMD58  0x7Au   /* READ_OCR          */
#define ACMD41 0x69u   /* SD_SEND_OP_COND   */

/* R1 status bits */
#define R1_IDLE        0x01u
#define R1_ILLEGAL_CMD 0x04u

/* Data tokens */
#define TOKEN_DATA_START       0xFEu
#define TOKEN_DATA_ACCEPT      0x05u
#define TOKEN_DATA_ACCEPT_MASK 0x1Fu

/* OCR bits */
#define OCR_CCS  ((uint32_t)1u << 30)  /* Card Capacity Status */

/* Convenience macros */
#define spi_init(s)         (s)->init(s)
#define spi_transfer(s, d)  (s)->transfer(s, d)
#define spi_csAssert(s)     (s)->csAssert(s)
#define spi_csDeassert(s)   (s)->csDeassert(s)
#define timer_delayMs(t, m) (t)->delayMs(t, m)

void SdCard_init(SdCard* self, ISpi* spi, ITimer* timer)
{
    memset(self, 0, sizeof(*self));
    self->spi   = spi;
    self->timer = timer;
}

uint8_t  SdCard_isHighCapacity(const SdCard* self) { return self->highCapacity; }
uint32_t SdCard_ocr(const SdCard* self)             { return self->ocr; }

/* Wait until the card sends a specific token byte. */
static uint8_t waitToken(SdCard* self, uint8_t token, uint32_t timeoutLoops)
{
    uint8_t r;
    do {
        r = spi_transfer(self->spi, 0xFF);
        if (r == token) return 1;
    } while (--timeoutLoops);
    return 0;
}

/* Send a 6-byte SPI SD command and return the R1 response byte.
 * CRC byte is pre-computed (only matters for CMD0 and CMD8). */
static uint8_t sendCommand(SdCard* self, uint8_t cmd, uint32_t arg)
{
    uint8_t crc = 0xFF;
    uint8_t r1;
    int i;

    if (cmd == CMD0) crc = 0x95u;
    if (cmd == CMD8) crc = 0x87u;

    spi_transfer(self->spi, 0xFF);  /* wait-ready cycle */

    spi_transfer(self->spi, cmd);
    spi_transfer(self->spi, (uint8_t)(arg >> 24));
    spi_transfer(self->spi, (uint8_t)(arg >> 16));
    spi_transfer(self->spi, (uint8_t)(arg >>  8));
    spi_transfer(self->spi, (uint8_t)(arg));
    spi_transfer(self->spi, crc);

    /* Wait for R1 (up to 8 retries – SD spec §7.5.1) */
    r1 = 0xFF;
    for (i = 0; i < 8; ++i) {
        r1 = spi_transfer(self->spi, 0xFF);
        if (!(r1 & 0x80u)) break;
    }
    return r1;
}

/* Send an application-specific command (preceded by CMD55). */
static uint8_t sendAcmd(SdCard* self, uint8_t cmd, uint32_t arg)
{
    sendCommand(self, CMD55, 0);
    return sendCommand(self, cmd, arg);
}

SdCardResult SdCard_begin(SdCard* self)
{
    uint8_t r1;
    uint8_t r7[4];
    uint8_t isV2 = 0;
    uint32_t acmd41arg;
    uint32_t retries;
    int i;

    spi_init(self->spi);
    self->highCapacity = 0;
    self->ocr = 0;

    /* SD card requires ≥74 clock cycles with CS de-asserted before any command */
    spi_csDeassert(self->spi);
    for (i = 0; i < 10; ++i) {
        spi_transfer(self->spi, 0xFF);
    }

    spi_csAssert(self->spi);

    /* CMD0: reset into SPI mode (expect R1 = 0x01 IDLE) */
    r1 = sendCommand(self, CMD0, 0);
    if (r1 != R1_IDLE) {
        spi_csDeassert(self->spi);
        return SDCARD_NOT_PRESENT;
    }

    /* CMD8: check for SD v2 (voltage range 2.7-3.6 V, check pattern 0xAA) */
    r1 = sendCommand(self, CMD8, 0x000001AAu);
    if (!(r1 & R1_ILLEGAL_CMD)) {
        /* SD v2: read 4-byte R7 tail */
        for (i = 0; i < 4; ++i) r7[i] = spi_transfer(self->spi, 0xFF);
        if (r7[2] == 0x01u && r7[3] == 0xAAu) {
            isV2 = 1;
        }
    }

    /* ACMD41: initialise card (with HCS bit set for v2) */
    acmd41arg = isV2 ? 0x40000000u : 0u;
    retries = 1000;
    do {
        r1 = sendAcmd(self, ACMD41, acmd41arg);
        if (r1 == 0) break;
        timer_delayMs(self->timer, 1);
    } while (--retries);

    if (r1 != 0) {
        spi_csDeassert(self->spi);
        return SDCARD_TIMEOUT;
    }

    /* CMD58: read OCR to determine SDHC vs SDSC */
    if (isV2) {
        r1 = sendCommand(self, CMD58, 0);
        if (r1 == 0) {
            self->ocr  = (uint32_t)spi_transfer(self->spi, 0xFF) << 24;
            self->ocr |= (uint32_t)spi_transfer(self->spi, 0xFF) << 16;
            self->ocr |= (uint32_t)spi_transfer(self->spi, 0xFF) <<  8;
            self->ocr |=           spi_transfer(self->spi, 0xFF);
            self->highCapacity = (self->ocr & OCR_CCS) ? 1u : 0u;
        }
    }

    spi_csDeassert(self->spi);
    spi_transfer(self->spi, 0xFF);  /* extra clock to finalise */
    return SDCARD_OK;
}

SdCardResult SdCard_readBlock(SdCard* self, uint32_t blockNum, uint8_t* buffer)
{
    uint32_t addr;
    uint8_t r1;
    size_t i;

    /* SDSC: byte addressing; SDHC: block addressing */
    addr = self->highCapacity ? blockNum : blockNum * SDCARD_BLOCK_SIZE;

    spi_csAssert(self->spi);
    r1 = sendCommand(self, CMD17, addr);
    if (r1 != 0) {
        spi_csDeassert(self->spi);
        return SDCARD_ERROR;
    }

    if (!waitToken(self, TOKEN_DATA_START, 500000u)) {
        spi_csDeassert(self->spi);
        return SDCARD_TIMEOUT;
    }

    for (i = 0; i < SDCARD_BLOCK_SIZE; ++i) {
        buffer[i] = spi_transfer(self->spi, 0xFF);
    }
    /* Discard 2 CRC bytes */
    spi_transfer(self->spi, 0xFF);
    spi_transfer(self->spi, 0xFF);

    spi_csDeassert(self->spi);
    spi_transfer(self->spi, 0xFF);
    return SDCARD_OK;
}

SdCardResult SdCard_writeBlock(SdCard* self, uint32_t blockNum,
                                const uint8_t* buffer)
{
    uint32_t addr;
    uint8_t r1, resp;
    uint32_t timeout;
    size_t i;

    addr = self->highCapacity ? blockNum : blockNum * SDCARD_BLOCK_SIZE;

    spi_csAssert(self->spi);
    r1 = sendCommand(self, CMD24, addr);
    if (r1 != 0) {
        spi_csDeassert(self->spi);
        return SDCARD_ERROR;
    }

    spi_transfer(self->spi, 0xFF);        /* short gap before data */
    spi_transfer(self->spi, TOKEN_DATA_START);

    for (i = 0; i < SDCARD_BLOCK_SIZE; ++i) {
        spi_transfer(self->spi, buffer[i]);
    }
    /* Dummy CRC */
    spi_transfer(self->spi, 0xFF);
    spi_transfer(self->spi, 0xFF);

    /* Read data response token */
    resp = spi_transfer(self->spi, 0xFF);
    if ((resp & TOKEN_DATA_ACCEPT_MASK) != TOKEN_DATA_ACCEPT) {
        spi_csDeassert(self->spi);
        return SDCARD_ERROR;
    }

    /* Wait for write to complete (busy = 0x00) */
    timeout = 500000u;
    while (spi_transfer(self->spi, 0xFF) == 0x00u) {
        if (--timeout == 0) {
            spi_csDeassert(self->spi);
            return SDCARD_TIMEOUT;
        }
    }

    spi_csDeassert(self->spi);
    spi_transfer(self->spi, 0xFF);
    return SDCARD_OK;
}
