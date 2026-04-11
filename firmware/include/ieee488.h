#pragma once
/* IEEE-488 / GPIB bus driver for PetDisk.
 * Implements the three-wire handshake as a listener/talker device.
 * All IEEE-488 lines are active-LOW; open-drain output is used throughout.
 */
#include "hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Timeout limit (loop iterations) for handshake operations. */
#define IEEE488_DEFAULT_TIMEOUT 100000u

/* Callback types ──────────────────────────────────────────────────────────── */

/* Invoked when a data byte arrives while addressed as listener.
 * eoi: 1 if EOI was asserted (last byte), 0 otherwise. */
typedef void (*Ieee488DataCallback)(uint8_t byte, uint8_t eoi, void* ctx);

/* Invoked when the controller wants to read a byte (talker mode).
 * *isLast should be set to 1 to assert EOI with the returned byte.
 * Return value: byte to place on bus, or -1 if no data available. */
typedef int  (*Ieee488TalkCallback)(uint8_t* isLast, void* ctx);

/* Ieee488 state ────────────────────────────────────────────────────────────── */
typedef struct {
    IGpioPort* dataBus;
    IGpioPin*  dav;
    IGpioPin*  nrfd;
    IGpioPin*  ndac;
    IGpioPin*  atn;
    IGpioPin*  eoi;
    IGpioPin*  srq;
    IGpioPin*  ifc;
    IGpioPin*  ren;
    uint8_t    deviceAddress;
    uint8_t    listenActive;
    uint8_t    talkActive;
    Ieee488DataCallback dataCb;
    void*               dataCbCtx;
    Ieee488TalkCallback talkCb;
    void*               talkCbCtx;
} Ieee488;

/* Initialise the struct with the given HAL objects.
 * Does NOT configure any hardware pins; call Ieee488_begin() for that. */
void Ieee488_init(Ieee488* self, IGpioPort* dataBus,
                  IGpioPin* dav, IGpioPin* nrfd, IGpioPin* ndac,
                  IGpioPin* atn, IGpioPin* eoi, IGpioPin* srq,
                  IGpioPin* ifc, IGpioPin* ren,
                  uint8_t deviceAddress);

/* Configure all bus pins and release the bus (call once at startup). */
void Ieee488_begin(Ieee488* self);

/* Call repeatedly from the main loop. */
void Ieee488_poll(Ieee488* self);

/* Send a single byte as talker.  isLastByte: 1 to assert EOI.
 * Returns 1 on success, 0 on timeout. */
int Ieee488_sendByte(Ieee488* self, uint8_t byte, uint8_t isLastByte);

/* Receive a single byte as listener.
 * *isLastByte is set to 1 if EOI was asserted.
 * Returns byte value (0-255) or -1 on error/timeout. */
int Ieee488_receiveByte(Ieee488* self, uint8_t* isLastByte);

uint8_t Ieee488_isAddressedToListen(const Ieee488* self);
uint8_t Ieee488_isAddressedToTalk(const Ieee488* self);
uint8_t Ieee488_isAttnActive(const Ieee488* self);

void Ieee488_assertSrq(Ieee488* self);
void Ieee488_releaseSrq(Ieee488* self);

void Ieee488_setDataCallback(Ieee488* self, Ieee488DataCallback cb, void* ctx);
void Ieee488_setTalkCallback(Ieee488* self, Ieee488TalkCallback cb, void* ctx);

#ifdef __cplusplus
} /* extern "C" */
#endif
