/* STM8S HAL implementation – compiled only with SDCC for the STM8 target.
 * Register addresses are for the STM8S105K6T6C.
 *
 * Each concrete implementation struct embeds the interface struct as its
 * FIRST member.  A pointer to the concrete struct may therefore be safely
 * cast to the interface pointer type, enabling C-based polymorphism.
 */
#ifdef __SDCC

#include "hal.h"
#include <stdint.h>
#include <string.h>

/* ── STM8S register definitions ─────────────────────────────────────────── */
/* Port A */
#define PA_ODR  (*(volatile uint8_t*)0x5000)
#define PA_IDR  (*(volatile uint8_t*)0x5001)
#define PA_DDR  (*(volatile uint8_t*)0x5002)
#define PA_CR1  (*(volatile uint8_t*)0x5003)
#define PA_CR2  (*(volatile uint8_t*)0x5004)

/* Port B */
#define PB_ODR  (*(volatile uint8_t*)0x5005)
#define PB_IDR  (*(volatile uint8_t*)0x5006)
#define PB_DDR  (*(volatile uint8_t*)0x5007)
#define PB_CR1  (*(volatile uint8_t*)0x5008)
#define PB_CR2  (*(volatile uint8_t*)0x5009)

/* Port C */
#define PC_ODR  (*(volatile uint8_t*)0x500A)
#define PC_IDR  (*(volatile uint8_t*)0x500B)
#define PC_DDR  (*(volatile uint8_t*)0x500C)
#define PC_CR1  (*(volatile uint8_t*)0x500D)
#define PC_CR2  (*(volatile uint8_t*)0x500E)

/* Port D */
#define PD_ODR  (*(volatile uint8_t*)0x500F)
#define PD_IDR  (*(volatile uint8_t*)0x5010)
#define PD_DDR  (*(volatile uint8_t*)0x5011)
#define PD_CR1  (*(volatile uint8_t*)0x5012)
#define PD_CR2  (*(volatile uint8_t*)0x5013)

/* SPI1 */
#define SPI_CR1   (*(volatile uint8_t*)0x5200)
#define SPI_CR2   (*(volatile uint8_t*)0x5201)
#define SPI_ICR   (*(volatile uint8_t*)0x5202)
#define SPI_SR    (*(volatile uint8_t*)0x5203)
#define SPI_DR    (*(volatile uint8_t*)0x5204)

#define SPI_SR_TXE   0x02u   /* Tx empty     */
#define SPI_SR_RXNE  0x01u   /* Rx not empty */
#define SPI_SR_BSY   0x80u   /* Busy         */

/* CLK */
#define CLK_CKDIVR (*(volatile uint8_t*)0x50C6)

/* TIM2 (used for delay) */
#define TIM2_CR1   (*(volatile uint8_t*)0x5300)
#define TIM2_IER   (*(volatile uint8_t*)0x5303)
#define TIM2_SR1   (*(volatile uint8_t*)0x5304)
#define TIM2_EGR   (*(volatile uint8_t*)0x5306)
#define TIM2_CCMR1 (*(volatile uint8_t*)0x5307)
#define TIM2_CNTRH (*(volatile uint8_t*)0x530E)
#define TIM2_CNTRL (*(volatile uint8_t*)0x530F)
#define TIM2_PSCR  (*(volatile uint8_t*)0x5310)
#define TIM2_ARRH  (*(volatile uint8_t*)0x5311)
#define TIM2_ARRL  (*(volatile uint8_t*)0x5312)
#define TIM2_SR1_UIF 0x01u

/* ── Stm8GpioPin ─────────────────────────────────────────────────────────── */

typedef struct {
    IGpioPin           iface;  /* MUST be first */
    volatile uint8_t*  odr;
    volatile uint8_t*  idr;
    volatile uint8_t*  ddr;
    volatile uint8_t*  cr1;
    uint8_t            bit;
} Stm8GpioPin;

static void Stm8GpioPin_setOutput(IGpioPin* p)
{
    Stm8GpioPin* self = (Stm8GpioPin*)p;
    *self->ddr |=  self->bit;   /* output    */
    *self->cr1 |=  self->bit;   /* push-pull */
}
static void Stm8GpioPin_setInput(IGpioPin* p)
{
    Stm8GpioPin* self = (Stm8GpioPin*)p;
    *self->ddr &= ~self->bit;   /* input    */
    *self->cr1 &= ~self->bit;   /* floating */
}
static void Stm8GpioPin_setOpenDrain(IGpioPin* p)
{
    Stm8GpioPin* self = (Stm8GpioPin*)p;
    *self->ddr |=  self->bit;
    *self->cr1 &= ~self->bit;   /* open-drain: DDR=1, CR1=0 */
}
static void Stm8GpioPin_setHigh(IGpioPin* p)
{
    Stm8GpioPin* self = (Stm8GpioPin*)p;
    *self->odr |=  self->bit;
}
static void Stm8GpioPin_setLow(IGpioPin* p)
{
    Stm8GpioPin* self = (Stm8GpioPin*)p;
    *self->odr &= ~self->bit;
}
static int Stm8GpioPin_read(const IGpioPin* p)
{
    const Stm8GpioPin* self = (const Stm8GpioPin*)p;
    return (*self->idr & self->bit) ? 1 : 0;
}

static void Stm8GpioPin_init(Stm8GpioPin* self,
                              volatile uint8_t* odr,
                              volatile uint8_t* idr,
                              volatile uint8_t* ddr,
                              volatile uint8_t* cr1,
                              uint8_t bit)
{
    self->iface.setOutput    = Stm8GpioPin_setOutput;
    self->iface.setInput     = Stm8GpioPin_setInput;
    self->iface.setOpenDrain = Stm8GpioPin_setOpenDrain;
    self->iface.setHigh      = Stm8GpioPin_setHigh;
    self->iface.setLow       = Stm8GpioPin_setLow;
    self->iface.read         = Stm8GpioPin_read;
    self->odr = odr;
    self->idr = idr;
    self->ddr = ddr;
    self->cr1 = cr1;
    self->bit = bit;
}

/* ── Stm8GpioPort (Port B – full 8-bit data bus) ──────────────────────────── */

typedef struct {
    IGpioPort         iface;   /* MUST be first */
    volatile uint8_t* odr;
    volatile uint8_t* idr;
    volatile uint8_t* ddr;
    volatile uint8_t* cr1;
} Stm8GpioPort;

static void Stm8GpioPort_setOutputMask(IGpioPort* p, uint8_t mask)
{
    Stm8GpioPort* self = (Stm8GpioPort*)p;
    *self->ddr |= mask;
    *self->cr1 |= mask;    /* push-pull */
}
static void Stm8GpioPort_setInputMask(IGpioPort* p, uint8_t mask)
{
    Stm8GpioPort* self = (Stm8GpioPort*)p;
    *self->ddr &= ~mask;
    *self->cr1 &= ~mask;
}
static void Stm8GpioPort_setOpenDrainMask(IGpioPort* p, uint8_t mask)
{
    Stm8GpioPort* self = (Stm8GpioPort*)p;
    *self->ddr |=  mask;
    *self->cr1 &= ~mask;   /* open-drain */
}
static void Stm8GpioPort_write(IGpioPort* p, uint8_t value)
{
    Stm8GpioPort* self = (Stm8GpioPort*)p;
    *self->odr = value;
}
static uint8_t Stm8GpioPort_read(const IGpioPort* p)
{
    const Stm8GpioPort* self = (const Stm8GpioPort*)p;
    return *self->idr;
}

static void Stm8GpioPort_init(Stm8GpioPort* self)
{
    self->iface.setOutputMask    = Stm8GpioPort_setOutputMask;
    self->iface.setInputMask     = Stm8GpioPort_setInputMask;
    self->iface.setOpenDrainMask = Stm8GpioPort_setOpenDrainMask;
    self->iface.write            = Stm8GpioPort_write;
    self->iface.read             = Stm8GpioPort_read;
    self->odr = &PB_ODR;
    self->idr = &PB_IDR;
    self->ddr = &PB_DDR;
    self->cr1 = &PB_CR1;
}

/* ── Stm8Spi ──────────────────────────────────────────────────────────────── */

typedef struct {
    ISpi iface;   /* MUST be first */
} Stm8Spi;

static void Stm8Spi_init(ISpi* p)
{
    (void)p;

    /* Enable 16 MHz HSI (default), no prescaler */
    CLK_CKDIVR = 0x00u;

    /* Configure SPI pins (PC5=SCK, PC6=MOSI output; PC7=MISO input) */
    PC_DDR |= (1u << 5) | (1u << 6);
    PC_CR1 |= (1u << 5) | (1u << 6);
    PC_DDR &= ~(1u << 7);

    /* CS pin PA3 */
    PA_DDR |= (1u << 3);
    PA_CR1 |= (1u << 3);
    PA_ODR |= (1u << 3);   /* CS de-asserted */

    /* SPI_CR1: SPE=1, MSTR=1, fPCLK/8 (BR=010), CPOL=0, CPHA=0 */
    SPI_CR1 = 0x34u;   /* 0b00110100 */
    SPI_CR2 = 0x00u;
}

static uint8_t Stm8Spi_transfer(ISpi* p, uint8_t data)
{
    (void)p;
    while (!(SPI_SR & SPI_SR_TXE)) {}
    SPI_DR = data;
    while (!(SPI_SR & SPI_SR_RXNE)) {}
    return SPI_DR;
}

static void Stm8Spi_csAssert(ISpi* p)
{
    (void)p;
    PA_ODR &= ~(1u << 3);
}

static void Stm8Spi_csDeassert(ISpi* p)
{
    (void)p;
    PA_ODR |= (1u << 3);
}

static void Stm8Spi_init_iface(Stm8Spi* self)
{
    self->iface.init      = Stm8Spi_init;
    self->iface.transfer  = Stm8Spi_transfer;
    self->iface.csAssert  = Stm8Spi_csAssert;
    self->iface.csDeassert = Stm8Spi_csDeassert;
}

/* ── Stm8Timer ────────────────────────────────────────────────────────────── */

typedef struct {
    ITimer iface;   /* MUST be first */
} Stm8Timer;

static void Stm8Timer_delayUs(ITimer* p, uint32_t us)
{
    (void)p;
    /* TIM2 configured for 1 µs tick at 16 MHz (prescaler = 16 → 1 MHz) */
    TIM2_CR1  = 0x00u;
    TIM2_PSCR = 0x04u;                           /* /16 → 1 MHz tick */
    TIM2_ARRH = (uint8_t)((us >> 8) & 0xFFu);
    TIM2_ARRL = (uint8_t)(us & 0xFFu);
    TIM2_SR1  = 0x00u;
    TIM2_EGR  = 0x01u;   /* generate update (reload prescaler) */
    TIM2_CR1  = 0x01u;   /* enable */
    while (!(TIM2_SR1 & TIM2_SR1_UIF)) {}
    TIM2_CR1  = 0x00u;
}

static void Stm8Timer_delayMs(ITimer* p, uint32_t ms)
{
    while (ms--) Stm8Timer_delayUs(p, 1000u);
}

static uint32_t Stm8Timer_tickMs(const ITimer* p)
{
    uint32_t hi, lo;
    (void)p;
    /* Simple 16-bit counter wrap – suitable for short timeouts only */
    hi = TIM2_CNTRH;
    lo = TIM2_CNTRL;
    return (hi << 8) | lo;
}

static void Stm8Timer_init(Stm8Timer* self)
{
    self->iface.delayMs = Stm8Timer_delayMs;
    self->iface.delayUs = Stm8Timer_delayUs;
    self->iface.tickMs  = Stm8Timer_tickMs;
}

/* ── Application-level objects (static storage, allocated once) ─────────── */

static Stm8GpioPort s_dataPort;

/* Port D pins: bit positions match the schematics */
static Stm8GpioPin s_dav,  s_nrfd, s_ndac;
static Stm8GpioPin s_atn,  s_eoi,  s_srq;
static Stm8GpioPin s_ifc,  s_ren;

static Stm8Spi   s_spi;
static Stm8Timer s_timer;

/* Initialise and return pointers to all HAL objects.
 * Called from main() before constructing PetDisk. */
void stm8_hal_init(IGpioPort** dataPort,
                   IGpioPin**  dav,   IGpioPin** nrfd, IGpioPin** ndac,
                   IGpioPin**  atn,   IGpioPin** eoi,  IGpioPin** srq,
                   IGpioPin**  ifc,   IGpioPin** ren,
                   ISpi**      spi,
                   ITimer**    timer)
{
    Stm8GpioPort_init(&s_dataPort);

    /* Port D bit masks */
    Stm8GpioPin_init(&s_dav,  &PD_ODR, &PD_IDR, &PD_DDR, &PD_CR1, 0x01u);
    Stm8GpioPin_init(&s_nrfd, &PD_ODR, &PD_IDR, &PD_DDR, &PD_CR1, 0x02u);
    Stm8GpioPin_init(&s_ndac, &PD_ODR, &PD_IDR, &PD_DDR, &PD_CR1, 0x04u);
    Stm8GpioPin_init(&s_atn,  &PD_ODR, &PD_IDR, &PD_DDR, &PD_CR1, 0x08u);
    Stm8GpioPin_init(&s_eoi,  &PD_ODR, &PD_IDR, &PD_DDR, &PD_CR1, 0x10u);
    Stm8GpioPin_init(&s_srq,  &PD_ODR, &PD_IDR, &PD_DDR, &PD_CR1, 0x20u);
    Stm8GpioPin_init(&s_ifc,  &PD_ODR, &PD_IDR, &PD_DDR, &PD_CR1, 0x40u);
    Stm8GpioPin_init(&s_ren,  &PD_ODR, &PD_IDR, &PD_DDR, &PD_CR1, 0x80u);

    Stm8Spi_init_iface(&s_spi);
    Stm8Timer_init(&s_timer);

    *dataPort = &s_dataPort.iface;
    *dav      = &s_dav.iface;
    *nrfd     = &s_nrfd.iface;
    *ndac     = &s_ndac.iface;
    *atn      = &s_atn.iface;
    *eoi      = &s_eoi.iface;
    *srq      = &s_srq.iface;
    *ifc      = &s_ifc.iface;
    *ren      = &s_ren.iface;
    *spi      = &s_spi.iface;
    *timer    = &s_timer.iface;
}

#endif /* __SDCC */
