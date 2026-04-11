#pragma once
/* HAL abstract interfaces for PetDisk firmware.
 * Implemented by hal_linux.cpp (simulation/test) and hal_stm8.c (target).
 * Each interface is a struct of function pointers (C vtable).
 * Concrete implementations embed the interface struct as their FIRST member
 * so that a pointer to the concrete struct may be cast to the interface type.
 */
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── GPIO single-pin abstraction ────────────────────────────────────────── */
typedef struct IGpioPin IGpioPin;
struct IGpioPin {
    void (*setOutput)(IGpioPin* self);
    void (*setInput)(IGpioPin* self);
    void (*setOpenDrain)(IGpioPin* self);   /* output that can only pull low */
    void (*setHigh)(IGpioPin* self);        /* release (open-drain: float high) */
    void (*setLow)(IGpioPin* self);         /* assert  (open-drain: pull to GND) */
    int  (*read)(const IGpioPin* self);     /* returns 1=HIGH, 0=LOW */
};

/* ── 8-bit GPIO port abstraction (IEEE-488 data bus) ────────────────────── */
typedef struct IGpioPort IGpioPort;
struct IGpioPort {
    void    (*setOutputMask)(IGpioPort* self, uint8_t mask);
    void    (*setInputMask)(IGpioPort* self, uint8_t mask);
    void    (*setOpenDrainMask)(IGpioPort* self, uint8_t mask);
    void    (*write)(IGpioPort* self, uint8_t value);
    uint8_t (*read)(const IGpioPort* self);
};

/* ── SPI bus abstraction (SD card) ──────────────────────────────────────── */
typedef struct ISpi ISpi;
struct ISpi {
    void    (*init)(ISpi* self);
    uint8_t (*transfer)(ISpi* self, uint8_t data);
    void    (*csAssert)(ISpi* self);
    void    (*csDeassert)(ISpi* self);
};

/* ── Timer / delay abstraction ───────────────────────────────────────────── */
typedef struct ITimer ITimer;
struct ITimer {
    void     (*delayMs)(ITimer* self, uint32_t ms);
    void     (*delayUs)(ITimer* self, uint32_t us);
    uint32_t (*tickMs)(const ITimer* self);
};

/* ── Filesystem abstraction (used by CbmDos) ─────────────────────────────── */
typedef struct IFilesystem IFilesystem;
struct IFilesystem {
    /* Open a file; channel 0 = sequential read, channel 1 = sequential write.
     * Returns 1 on success, 0 on failure. */
    int         (*open)(IFilesystem* self, uint8_t channel,
                        const char* filename, uint8_t mode);
    void        (*close)(IFilesystem* self, uint8_t channel);

    /* Read up to maxLen bytes into buf; returns bytes read (0 = EOF). */
    size_t      (*read)(IFilesystem* self, uint8_t channel,
                        uint8_t* buf, size_t maxLen);

    /* Write len bytes from buf; returns bytes written. */
    size_t      (*write)(IFilesystem* self, uint8_t channel,
                         const uint8_t* buf, size_t len);

    /* CBM error code for last error (0 = OK). */
    uint8_t     (*errorCode)(const IFilesystem* self);

    /* Human-readable message for last error. */
    const char* (*errorMessage)(const IFilesystem* self);

    /* Execute a command string (e.g. "S:file", "R:old=new"). */
    void        (*executeCommand)(IFilesystem* self,
                                  const char* cmd, size_t len);
};

#ifdef __cplusplus
} /* extern "C" */
#endif
