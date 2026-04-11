/* IEEE-488 / GPIB three-wire handshake driver for PetDisk.
 * All bus signals are active-LOW; open-drain outputs are used throughout.
 * "assert" means drive LOW; "release" means float HIGH (open-drain).
 */
#include "ieee488.h"
#include <string.h>

/* IEEE-488 listen/talk address offsets */
#define kListenAddr 0x20u   /* MLA: My Listen Address */
#define kTalkAddr   0x40u   /* MTA: My Talk Address   */
#define kUnlisten   0x3Fu
#define kUntalk     0x5Fu

/* Convenience macros to call through function-pointer vtable */
#define pin_setOutput(p)      (p)->setOutput(p)
#define pin_setInput(p)       (p)->setInput(p)
#define pin_setOpenDrain(p)   (p)->setOpenDrain(p)
#define pin_setHigh(p)        (p)->setHigh(p)
#define pin_setLow(p)         (p)->setLow(p)
#define pin_read(p)           (p)->read(p)

#define port_setOutputMask(p, m)    (p)->setOutputMask(p, m)
#define port_setInputMask(p, m)     (p)->setInputMask(p, m)
#define port_setOpenDrainMask(p, m) (p)->setOpenDrainMask(p, m)
#define port_write(p, v)            (p)->write(p, v)
#define port_read(p)                (p)->read(p)

void Ieee488_init(Ieee488* self, IGpioPort* dataBus,
                  IGpioPin* dav, IGpioPin* nrfd, IGpioPin* ndac,
                  IGpioPin* atn, IGpioPin* eoi, IGpioPin* srq,
                  IGpioPin* ifc, IGpioPin* ren,
                  uint8_t deviceAddress)
{
    memset(self, 0, sizeof(*self));
    self->dataBus       = dataBus;
    self->dav           = dav;
    self->nrfd          = nrfd;
    self->ndac          = ndac;
    self->atn           = atn;
    self->eoi           = eoi;
    self->srq           = srq;
    self->ifc           = ifc;
    self->ren           = ren;
    self->deviceAddress = deviceAddress;
}

void Ieee488_begin(Ieee488* self)
{
    pin_setOpenDrain(self->dav);  pin_setHigh(self->dav);   /* release DAV  */
    pin_setOpenDrain(self->nrfd); pin_setHigh(self->nrfd);  /* release NRFD */
    pin_setOpenDrain(self->ndac); pin_setHigh(self->ndac);  /* release NDAC */
    pin_setOpenDrain(self->eoi);  pin_setHigh(self->eoi);   /* release EOI  */
    pin_setOpenDrain(self->srq);  pin_setHigh(self->srq);   /* release SRQ  */

    /* ATN, IFC, REN are controller outputs – we only listen */
    pin_setInput(self->atn);
    pin_setInput(self->ifc);
    pin_setInput(self->ren);

    /* Data bus: open-drain, release all lines */
    port_setOpenDrainMask(self->dataBus, 0xFF);
    port_write(self->dataBus, 0xFF);   /* all HIGH = release */
}

uint8_t Ieee488_isAttnActive(const Ieee488* self)
{
    return !pin_read(self->atn);   /* active LOW */
}

void Ieee488_assertSrq(Ieee488* self)  { pin_setLow(self->srq);  }
void Ieee488_releaseSrq(Ieee488* self) { pin_setHigh(self->srq); }

void Ieee488_setDataCallback(Ieee488* self, Ieee488DataCallback cb, void* ctx)
{
    self->dataCb    = cb;
    self->dataCbCtx = ctx;
}

void Ieee488_setTalkCallback(Ieee488* self, Ieee488TalkCallback cb, void* ctx)
{
    self->talkCb    = cb;
    self->talkCbCtx = ctx;
}

uint8_t Ieee488_isAddressedToListen(const Ieee488* self) { return self->listenActive; }
uint8_t Ieee488_isAddressedToTalk(const Ieee488* self)   { return self->talkActive;  }

/* Release (float) all data lines so another talker can use the bus. */
static void releaseDataBus(Ieee488* self)
{
    port_write(self->dataBus, 0xFF);
}

/* Drive inverted data value onto open-drain bus.
 * IEEE-488 data lines are active-LOW: '1' bit = line LOW, '0' bit = line HIGH. */
static void driveDataBus(Ieee488* self, uint8_t byte)
{
    port_write(self->dataBus, (uint8_t)~byte);
}

/* Handle ATN commands sent by the controller.
 * ATN commands are single bytes:
 *   0x20-0x3E  MLA (My Listen Address)
 *   0x3F       Unlisten
 *   0x40-0x5E  MTA (My Talk Address)
 *   0x5F       Untalk
 *   0x60-0x6F  Secondary address
 */
static void handleAttn(Ieee488* self)
{
    uint8_t last = 0;
    int b = Ieee488_receiveByte(self, &last);
    if (b < 0) return;

    uint8_t cmd = (uint8_t)b;

    if (cmd == kUnlisten) {
        self->listenActive = 0;
        releaseDataBus(self);
        return;
    }
    if (cmd == kUntalk) {
        self->talkActive = 0;
        releaseDataBus(self);
        return;
    }
    if (cmd == (uint8_t)(kListenAddr | (self->deviceAddress & 0x1Fu))) {
        self->listenActive = 1;
        self->talkActive   = 0;
        return;
    }
    if (cmd == (uint8_t)(kTalkAddr | (self->deviceAddress & 0x1Fu))) {
        self->talkActive   = 1;
        self->listenActive = 0;
        return;
    }
    /* Other addresses or secondary addresses – not for us, ignore. */
}

/* poll() is called repeatedly from the main loop. */
void Ieee488_poll(Ieee488* self)
{
    /* IFC resets the device */
    if (!pin_read(self->ifc)) {
        self->listenActive = 0;
        self->talkActive   = 0;
        releaseDataBus(self);
        pin_setHigh(self->nrfd);
        pin_setHigh(self->ndac);
        return;
    }

    if (Ieee488_isAttnActive(self)) {
        handleAttn(self);
        return;
    }

    if (self->listenActive && self->dataCb != NULL) {
        uint8_t eoi = 0;
        int b = Ieee488_receiveByte(self, &eoi);
        if (b >= 0) {
            self->dataCb((uint8_t)b, eoi, self->dataCbCtx);
        }
    }

    if (self->talkActive && self->talkCb != NULL) {
        uint8_t isLast = 0;
        int b = self->talkCb(&isLast, self->talkCbCtx);
        if (b >= 0) {
            Ieee488_sendByte(self, (uint8_t)b, isLast);
        }
    }
}

/* Receive one byte using the three-wire handshake (listener side).
 * Sequence:
 *   1. Assert NDAC (not accepted), release NRFD (ready)
 *   2. Wait for DAV LOW (talker has valid data)
 *   3. Assert NRFD (not ready), read data, release NDAC (accepted)
 *   4. Wait for DAV HIGH (talker acknowledged acceptance)
 *   5. Release NRFD
 */
int Ieee488_receiveByte(Ieee488* self, uint8_t* isLastByte)
{
    uint32_t timeout;

    /* Assert NDAC – we have not accepted previous data */
    pin_setLow(self->ndac);
    /* Release NRFD – we are ready for data */
    pin_setHigh(self->nrfd);

    /* Wait for DAV LOW (talker asserts data valid) */
    timeout = IEEE488_DEFAULT_TIMEOUT;
    while (pin_read(self->dav)) {
        if (--timeout == 0) {
            pin_setHigh(self->ndac);
            return -1;
        }
    }

    /* Assert NRFD – not ready for next byte yet */
    pin_setLow(self->nrfd);

    /* Read data bus (active-LOW on bus, invert for actual data value) */
    uint8_t data = (uint8_t)~port_read(self->dataBus);

    /* Check EOI (active LOW) */
    *isLastByte = !pin_read(self->eoi);

    /* Release NDAC – accepted */
    pin_setHigh(self->ndac);

    /* Wait for DAV to go HIGH (talker has seen our acceptance) */
    timeout = IEEE488_DEFAULT_TIMEOUT;
    while (!pin_read(self->dav)) {
        if (--timeout == 0) {
            return (int)data;
        }
    }

    /* Release NRFD – ready for next byte */
    pin_setHigh(self->nrfd);

    return (int)data;
}

/* Send one byte using the three-wire handshake (talker side).
 * Sequence:
 *   1. Wait for NRFD HIGH (listener ready) and NDAC LOW (not yet accepted)
 *   2. Place data on bus, optionally assert EOI
 *   3. Assert DAV LOW (data valid)
 *   4. Wait for NRFD LOW (listener busy) then NDAC HIGH (all accepted)
 *   5. Release DAV, release data, release EOI
 * Returns 1 on success, 0 on timeout.
 */
int Ieee488_sendByte(Ieee488* self, uint8_t byte, uint8_t isLastByte)
{
    uint32_t timeout;
    volatile uint8_t settle;

    /* Wait for NRFD HIGH (all listeners ready) */
    timeout = IEEE488_DEFAULT_TIMEOUT;
    while (!pin_read(self->nrfd)) {
        if (--timeout == 0) return 0;
    }

    /* Place data (inverted for active-LOW bus) */
    driveDataBus(self, byte);

    /* Assert EOI if this is the last byte */
    if (isLastByte) {
        pin_setLow(self->eoi);
    }

    /* Small settling – spin a few cycles */
    for (settle = 0; settle < 4; ++settle) { /* settling delay */ }

    /* Assert DAV – data is valid */
    pin_setLow(self->dav);

    /* Wait for NRFD LOW (listeners are processing) */
    timeout = IEEE488_DEFAULT_TIMEOUT;
    while (pin_read(self->nrfd)) {
        if (--timeout == 0) {
            pin_setHigh(self->dav);
            releaseDataBus(self);
            pin_setHigh(self->eoi);
            return 0;
        }
    }

    /* Wait for NDAC HIGH (all listeners have accepted) */
    timeout = IEEE488_DEFAULT_TIMEOUT;
    while (!pin_read(self->ndac)) {
        if (--timeout == 0) {
            pin_setHigh(self->dav);
            releaseDataBus(self);
            pin_setHigh(self->eoi);
            return 0;
        }
    }

    /* Release DAV, data bus, EOI */
    pin_setHigh(self->dav);
    releaseDataBus(self);
    if (isLastByte) {
        pin_setHigh(self->eoi);
    }

    return 1;
}
