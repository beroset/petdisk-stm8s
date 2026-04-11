#pragma once
/* SPI-mode SD card driver for PetDisk.
 * Supports SD v1, v2 and SDHC; CRC disabled for speed.
 */
#include "hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SDCARD_BLOCK_SIZE 512u

typedef enum {
    SDCARD_OK = 0,
    SDCARD_ERROR,
    SDCARD_TIMEOUT,
    SDCARD_NOT_PRESENT
} SdCardResult;

typedef struct {
    ISpi*    spi;
    ITimer*  timer;
    uint8_t  highCapacity;
    uint32_t ocr;
} SdCard;

/* Initialise the SdCard struct with HAL objects (constructor equivalent). */
void SdCard_init(SdCard* self, ISpi* spi, ITimer* timer);

/* Run the SD initialisation sequence.  Must be called before readBlock/writeBlock. */
SdCardResult SdCard_begin(SdCard* self);

/* Read one 512-byte block. */
SdCardResult SdCard_readBlock(SdCard* self, uint32_t blockNum, uint8_t* buffer);

/* Write one 512-byte block. */
SdCardResult SdCard_writeBlock(SdCard* self, uint32_t blockNum,
                                const uint8_t* buffer);

/* 1 if card is SDHC/SDXC (block addressing), 0 if byte addressing. */
uint8_t  SdCard_isHighCapacity(const SdCard* self);

/* OCR value read during init (useful for diagnostics). */
uint32_t SdCard_ocr(const SdCard* self);

#ifdef __cplusplus
} /* extern "C" */
#endif
