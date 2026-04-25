#ifndef STM8_H
#define STM8_H
#include <stdint.h>
// these definitions are from the STM8S207xx STM8S208xx datasheet

// FLASH
__at(0x505A) volatile uint8_t FLASH_CR1; //  Flash control register 1 (reset: 0x00)
__at(0x505B) volatile uint8_t FLASH_CR2; //  Flash control register 2 (reset: 0x00)
__at(0x505C) volatile uint8_t FLASH_NCR2; //  Flash complementary control register 2 (reset: 0xFF)
__at(0x505D) volatile uint8_t FLASH_FPR; //  Flash protection register (reset: 0x00)
__at(0x505E) volatile uint8_t FLASH_NFPR; //  Flash complementary protection register (reset: 0xFF)
__at(0x505F) volatile uint8_t FLASH_IAPSR; //  Flash in-application programming status register (reset: 0x00)
__at(0x5062) volatile uint8_t FLASH_PUKR; //  Flash Program memory unprotection register (reset: 0x00)
__at(0x5064) volatile uint8_t FLASH_DUKR; //  Data EEPROM unprotection register (reset: 0x00)

// ITC
__at(0x50A0) volatile uint8_t EXTI_CR1; //  External interrupt control register 1 (reset: 0x00)
__at(0x50A1) volatile uint8_t EXTI_CR2; //  External interrupt control register 2 (reset: 0x00)

// RST
__at(0x50B3) volatile uint8_t RST_SR; //  Reset status register (reset: 0xXX)

// CLK
__at(0x50C0) volatile uint8_t CLK_ICKR; //  Internal clock control register (reset: 0x01)
__at(0x50C1) volatile uint8_t CLK_ECKR; //  External clock control register (reset: 0x00)
// 0x50c2 - reserved
__at(0x50C3) volatile uint8_t CLK_CMSR; //  Clock master status register (reset: 0xE1)
__at(0x50C4) volatile uint8_t CLK_SWR; //  Clock master switch register (reset: 0xE1)
__at(0x50C5) volatile uint8_t CLK_SWCR; //  Clock switch control register (reset: 0xXX)
__at(0x50C6) volatile uint8_t CLK_CKDIVR; // Clock divider register (reset: 0x18)
__at(0x50C7) volatile uint8_t CLK_PCKENR1; //  Peripheral clock gating register 1 (reset: 0xFF)
__at(0x50C8) volatile uint8_t CLK_CSSR; //  Clock security system register (reset: 0x00)
__at(0x50C9) volatile uint8_t CLK_CCOR; //  Configurable clock control register (reset: 0x00)
__at(0x50CA) volatile uint8_t CLK_PCKENR2; //  Peripheral clock gating register 2 (reset: 0xFF)
__at(0x50CB) volatile uint8_t CLK_CANCCR; //  CAN clock control register (reset: 0x00)
__at(0x50CC) volatile uint8_t CLK_HSITRIMR; //  HSI clock calibration trimming register (reset: 0x00)
__at(0x50CD) volatile uint8_t CLK_SWIMCCR; //  SWIM clock control register (reset: 0bXXXX XXX0)

// WWDG
__at(0x50D1) volatile uint8_t WWDG_CR; //  WWDG control register (reset: 0x7F)
__at(0x50D2) volatile uint8_t WWDG_WR; //  WWDR window register (reset: 0x7F)

// IWDG
__at(0x50E0) volatile uint8_t IWDG_KR; //  IWDG key register (reset: 0xXX)
__at(0x50E1) volatile uint8_t IWDG_PR; //  IWDG prescaler register (reset: 0x00)
__at(0x50E2) volatile uint8_t IWDG_RLR; //  IWDG reload register (reset: 0xFF)

// AWU
__at(0x50F0) volatile uint8_t AWU_CSR1; //  AWU control/status register 1 (reset: 0x00)
__at(0x50F1) volatile uint8_t AWU_APR; //  AWU asynchronous prescaler buffer register (reset: 0x3F)
__at(0x50F2) volatile uint8_t AWU_TBR; //  AWU timebase selection register (reset: 0x00)

// BEEP
__at(0x50F3) volatile uint8_t BEEP_CSR; //  BEEP control/status register (reset: 0x1F)

// SPI
__at(0x5200) volatile uint8_t SPI_CR1; //  SPI control register 1 (reset: 0x00)
__at(0x5201) volatile uint8_t SPI_CR2; //  SPI control register 2 (reset: 0x00)
__at(0x5202) volatile uint8_t SPI_ICR; //  SPI interrupt control register (reset: 0x00)
__at(0x5203) volatile uint8_t SPI_SR; //  SPI status register (reset: 0x02)
__at(0x5204) volatile uint8_t SPI_DR; //  SPI data register (reset: 0x00)
__at(0x5205) volatile uint8_t SPI_CRCPR; //  SPI CRC polynomial register (reset: 0x07)
__at(0x5206) volatile uint8_t SPI_RXCRCR; //  SPI Rx CRC register (reset: 0xFF)
__at(0x5207) volatile uint8_t SPI_TXCRCR; //  SPI Tx CRC register (reset: 0xFF)

// I2C
__at(0x5210) volatile uint8_t I2C_CR1; //  I2C control register 1 (reset: 0x00)
__at(0x5211) volatile uint8_t I2C_CR2; //  I2C control register 2 (reset: 0x00)
__at(0x5212) volatile uint8_t I2C_FREQR; //  I2C frequency register (reset: 0x00)
__at(0x5213) volatile uint8_t I2C_OARL; //  I2C own address register low (reset: 0x00)
__at(0x5214) volatile uint8_t I2C_OARH; //  I2C own address register high (reset: 0x00)
__at(0x5216) volatile uint8_t I2C_DR; //  I2C data register (reset: 0x00)
__at(0x5217) volatile uint8_t I2C_SR1; //  I2C status register 1 (reset: 0x00)
__at(0x5218) volatile uint8_t I2C_SR2; //  I2C status register 2 (reset: 0x00)
__at(0x5219) volatile uint8_t I2C_SR3; //  I2C status register 3 (reset: 0x00)
__at(0x521A) volatile uint8_t I2C_ITR; //  I2C interrupt control register (reset: 0x00)
__at(0x521B) volatile uint8_t I2C_CCRL; //  I2C clock control register low (reset: 0x00)
__at(0x521C) volatile uint8_t I2C_CCRH; //  I2C clock control register high (reset: 0x00)
__at(0x521D) volatile uint8_t I2C_TRISER; //  I2C TRISE register (reset: 0x02)

// documented for STM8S105x4/6 only
__at(0x521E) volatile uint8_t I2C_PECR; //  I2C packet error checking register (reset: 0x00)

// UART1
__at(0x5230) volatile uint8_t UART1_SR; //  UART1 status register (reset: 0xC0)
__at(0x5231) volatile uint8_t UART1_DR; //  UART1 data register (reset: 0xXX)
__at(0x5232) volatile uint8_t UART1_BRR1; //  UART1 baud rate register 1 (reset: 0x00)
__at(0x5233) volatile uint8_t UART1_BRR2; //  UART1 baud rate register 2 (reset: 0x00)
__at(0x5234) volatile uint8_t UART1_CR1; //  UART1 control register 1 (reset: 0x00)
__at(0x5235) volatile uint8_t UART1_CR2; //  UART1 control register 2 (reset: 0x00)
__at(0x5236) volatile uint8_t UART1_CR3; //  UART1 control register 3 (reset: 0x00)
__at(0x5237) volatile uint8_t UART1_CR4; //  UART1 control register 4 (reset: 0x00)
__at(0x5238) volatile uint8_t UART1_CR5; //  UART1 control register 5 (reset: 0x00)
__at(0x5239) volatile uint8_t UART1_GTR; //  UART1 guard time register (reset: 0x00)
__at(0x523A) volatile uint8_t UART1_PSCR; //  UART1 prescaler register (reset: 0x00)

// this is called UART2 for STM8S105x4/6
// UART3
__at(0x5240) volatile uint8_t UART3_SR; //  UART3 status register (reset: 0xC0)
__at(0x5241) volatile uint8_t UART3_DR; //  UART3 data register (reset: 0xXX)
__at(0x5242) volatile uint8_t UART3_BRR1; //  UART3 baud rate register 1 (reset: 0x00)
__at(0x5243) volatile uint8_t UART3_BRR2; //  UART3 baud rate register 2 (reset: 0x00)
__at(0x5244) volatile uint8_t UART3_CR1; //  UART3 control register 1 (reset: 0x00)
__at(0x5245) volatile uint8_t UART3_CR2; //  UART3 control register 2 (reset: 0x00)
__at(0x5246) volatile uint8_t UART3_CR3; //  UART3 control register 3 (reset: 0x00)
__at(0x5247) volatile uint8_t UART3_CR4; //  UART3 control register 4 (reset: 0x00)
// 0x5248 reserved
__at(0x5249) volatile uint8_t UART3_CR6; //  UART3 control register 6 (reset: 0x00)

// TIM1
__at(0x5250) volatile uint8_t TIM1_CR1; //  TIM1 control register 1 (reset: 0x00)
__at(0x5251) volatile uint8_t TIM1_CR2; //  TIM1 control register 2 (reset: 0x00)
__at(0x5252) volatile uint8_t TIM1_SMCR; //  TIM1 slave mode control register (reset: 0x00)
__at(0x5253) volatile uint8_t TIM1_ETR; //  TIM1 external trigger register (reset: 0x00)
__at(0x5254) volatile uint8_t TIM1_IER; //  TIM1 Interrupt enable register (reset: 0x00)
__at(0x5255) volatile uint8_t TIM1_SR1; //  TIM1 status register 1 (reset: 0x00)
__at(0x5256) volatile uint8_t TIM1_SR2; //  TIM1 status register 2 (reset: 0x00)
__at(0x5257) volatile uint8_t TIM1_EGR; //  TIM1 event generation register (reset: 0x00)
__at(0x5258) volatile uint8_t TIM1_CCMR1; //  TIM1 capture/compare mode register 1 (reset: 0x00)
__at(0x5259) volatile uint8_t TIM1_CCMR2; //  TIM1 capture/compare mode register 2 (reset: 0x00)
__at(0x525A) volatile uint8_t TIM1_CCMR3; //  TIM1 capture/compare mode register 3 (reset: 0x00)
__at(0x525B) volatile uint8_t TIM1_CCMR4; //  TIM1 capture/compare mode register 4 (reset: 0x00)
__at(0x525C) volatile uint8_t TIM1_CCER1; //  TIM1 capture/compare enable register 1 (reset: 0x00)
__at(0x525D) volatile uint8_t TIM1_CCER2; //  TIM1 capture/compare enable register 2 (reset: 0x00)
__at(0x525E) volatile uint8_t TIM1_CNTRH; //  TIM1 counter high (reset: 0x00)
__at(0x525F) volatile uint8_t TIM1_CNTRL; //  TIM1 counter low (reset: 0x00)
__at(0x5260) volatile uint8_t TIM1_PSCRH; //  TIM1 prescaler register high (reset: 0x00)
__at(0x5261) volatile uint8_t TIM1_PSCRL; //  TIM1 prescaler register low (reset: 0x00)
__at(0x5262) volatile uint8_t TIM1_ARRH; //  TIM1 auto-reload register high (reset: 0xFF)
__at(0x5263) volatile uint8_t TIM1_ARRL; //  TIM1 auto-reload register low (reset: 0xFF)
__at(0x5264) volatile uint8_t TIM1_RCR; //  TIM1 repetition counter register (reset: 0x00)
__at(0x5265) volatile uint8_t TIM1_CCR1H; //  TIM1 capture/compare register 1 high (reset: 0x00)
__at(0x5266) volatile uint8_t TIM1_CCR1L; //  TIM1 capture/compare register 1 low (reset: 0x00)
__at(0x5267) volatile uint8_t TIM1_CCR2H; //  TIM1 capture/compare register 2 high (reset: 0x00)
__at(0x5268) volatile uint8_t TIM1_CCR2L; //  TIM1 capture/compare register 2 low (reset: 0x00)
__at(0x5269) volatile uint8_t TIM1_CCR3H; //  TIM1 capture/compare register 3 high (reset: 0x00)
__at(0x526A) volatile uint8_t TIM1_CCR3L; //  TIM1 capture/compare register 3 low (reset: 0x00)
__at(0x526B) volatile uint8_t TIM1_CCR4H; //  TIM1 capture/compare register 4 high (reset: 0x00)
__at(0x526C) volatile uint8_t TIM1_CCR4L; //  TIM1 capture/compare register 4 low (reset: 0x00)
__at(0x526D) volatile uint8_t TIM1_BKR; //  TIM1 break register (reset: 0x00)
__at(0x526E) volatile uint8_t TIM1_DTR; //  TIM1 dead-time register (reset: 0x00)
__at(0x526F) volatile uint8_t TIM1_OISR; //  TIM1 output idle state register (reset: 0x00)

// TIM2
__at(0x5300) volatile uint8_t TIM2_CR1; //  TIM2 control register 1 (reset: 0x00)
__at(0x5301) volatile uint8_t TIM2_IER; //  TIM2 interrupt enable register (reset: 0x00)
__at(0x5302) volatile uint8_t TIM2_SR1; //  TIM2 status register 1 (reset: 0x00)
__at(0x5303) volatile uint8_t TIM2_SR2; //  TIM2 status register 2 (reset: 0x00)
__at(0x5304) volatile uint8_t TIM2_EGR; //  TIM2 event generation register (reset: 0x00)
__at(0x5305) volatile uint8_t TIM2_CCMR1; //  TIM2 capture/compare mode register 1 (reset: 0x00)
__at(0x5306) volatile uint8_t TIM2_CCMR2; //  TIM2 capture/compare mode register 2 (reset: 0x00)
__at(0x5307) volatile uint8_t TIM2_CCMR3; //  TIM2 capture/compare mode register 3 (reset: 0x00)
__at(0x5308) volatile uint8_t TIM2_CCER1; //  TIM2 capture/compare enable register 1 (reset: 0x00)
__at(0x5309) volatile uint8_t TIM2_CCER2; //  TIM2 capture/compare enable register 2 (reset: 0x00)
__at(0x530A) volatile uint8_t TIM2_CNTRH; //  TIM2 counter high (reset: 0x00)
__at(0x530B) volatile uint8_t TIM2_CNTRL; //  TIM2 counter low (reset: 0x00)
__at(0x530C) volatile uint8_t TIM2_PSCR; //  TIM2 prescaler register (reset: 0x00)
__at(0x530D) volatile uint8_t TIM2_ARRH; //  TIM2 auto-reload register high (reset: 0xFF)
__at(0x530E) volatile uint8_t TIM2_ARRL; //  TIM2 auto-reload register low (reset: 0xFF)
__at(0x530F) volatile uint8_t TIM2_CCR1H; //  TIM2 capture/compare register 1 high (reset: 0x00)
__at(0x5310) volatile uint8_t TIM2_CCR1L; //  TIM2 capture/compare register 1 low (reset: 0x00)
__at(0x5311) volatile uint8_t TIM2_CCR2H; //  TIM2 capture/compare reg. 2 high (reset: 0x00)
__at(0x5312) volatile uint8_t TIM2_CCR2L; //  TIM2 capture/compare register 2 low (reset: 0x00)
__at(0x5313) volatile uint8_t TIM2_CCR3H; //  TIM2 capture/compare register 3 high (reset: 0x00)
__at(0x5314) volatile uint8_t TIM2_CCR3L; //  TIM2 capture/compare register 3 low (reset: 0x00)

// TIM3
__at(0x5320) volatile uint8_t TIM3_CR1; //  TIM3 control register 1 (reset: 0x00)
__at(0x5321) volatile uint8_t TIM3_IER; //  TIM3 interrupt enable register (reset: 0x00)
__at(0x5322) volatile uint8_t TIM3_SR1; //  TIM3 status register 1 (reset: 0x00)
__at(0x5323) volatile uint8_t TIM3_SR2; //  TIM3 status register 2 (reset: 0x00)
__at(0x5324) volatile uint8_t TIM3_EGR; //  TIM3 event generation register (reset: 0x00)
__at(0x5325) volatile uint8_t TIM3_CCMR1; //  TIM3 capture/compare mode register 1 (reset: 0x00)
__at(0x5326) volatile uint8_t TIM3_CCMR2; //  TIM3 capture/compare mode register 2 (reset: 0x00)
__at(0x5327) volatile uint8_t TIM3_CCER1; //  TIM3 capture/compare enable register 1 (reset: 0x00)
__at(0x5328) volatile uint8_t TIM3_CNTRH; //  TIM3 counter high (reset: 0x00)
__at(0x5329) volatile uint8_t TIM3_CNTRL; //  TIM3 counter low (reset: 0x00)
__at(0x532A) volatile uint8_t TIM3_PSCR; //  TIM3 prescaler register (reset: 0x00)
__at(0x532B) volatile uint8_t TIM3_ARRH; //  TIM3 auto-reload register high (reset: 0xFF)
__at(0x532C) volatile uint8_t TIM3_ARRL; //  TIM3 auto-reload register low (reset: 0xFF)
__at(0x532D) volatile uint8_t TIM3_CCR1H; //  TIM3 capture/compare register 1 high (reset: 0x00)
__at(0x532E) volatile uint8_t TIM3_CCR1L; //  TIM3 capture/compare register 1 low (reset: 0x00)
__at(0x532F) volatile uint8_t TIM3_CCR2H; //  TIM3 capture/compare register 2 high (reset: 0x00)
__at(0x5330) volatile uint8_t TIM3_CCR2L; //  TIM3 capture/compare register 2 low (reset: 0x00)

// TIM4
__at(0x5340) volatile uint8_t TIM4_CR1; //  TIM4 control register 1 (reset: 0x00)
__at(0x5341) volatile uint8_t TIM4_IER; //  TIM4 interrupt enable register (reset: 0x00)
__at(0x5342) volatile uint8_t TIM4_SR; //  TIM4 status register (reset: 0x00)
__at(0x5343) volatile uint8_t TIM4_EGR; //  TIM4 event generation register (reset: 0x00)
__at(0x5344) volatile uint8_t TIM4_CNTR; //  TIM4 counter (reset: 0x00)
__at(0x5345) volatile uint8_t TIM4_PSCR; //  TIM4 prescaler register (reset: 0x00)
__at(0x5346) volatile uint8_t TIM4_ARR; //  TIM4 auto-reload register (reset: 0xFF)

// at 0x53e0 is ADC1 data buffer registers for STM8S105x4/6
// ADC2
__at(0x5400) volatile uint8_t ADC_CSR; //  ADC control/status register (reset: 0x00)
__at(0x5401) volatile uint8_t ADC_CR1; //  ADC configuration register 1 (reset: 0x00)
__at(0x5402) volatile uint8_t ADC_CR2; //  ADC configuration register 2 (reset: 0x00)
__at(0x5403) volatile uint8_t ADC_CR3; //  ADC configuration register 3 (reset: 0x00)
__at(0x5404) volatile uint8_t ADC_DRH; //  ADC data register high (reset: 0xXX)
__at(0x5405) volatile uint8_t ADC_DRL; //  ADC data register low (reset: 0xXX)
__at(0x5406) volatile uint8_t ADC_TDRH; //  ADC Schmitt trigger disable register high (reset: 0x00)
__at(0x5407) volatile uint8_t ADC_TDRL; //  ADC Schmitt trigger disable register low (reset: 0x00)

// no CAN for STM8S105x4/6
// beCAN
__at(0x5420) volatile uint8_t CAN_MCR; //  CAN master control register (reset: 0x02)
__at(0x5421) volatile uint8_t CAN_MSR; //  CAN master status register (reset: 0x02)
__at(0x5422) volatile uint8_t CAN_TSR; //  CAN transmit status register (reset: 0x00)
__at(0x5423) volatile uint8_t CAN_TPR; //  CAN transmit priority register (reset: 0x0C)
__at(0x5424) volatile uint8_t CAN_RFR; //  CAN receive FIFO register (reset: 0x00)
__at(0x5425) volatile uint8_t CAN_IER; //  CAN interrupt enable register (reset: 0x00)
__at(0x5426) volatile uint8_t CAN_DGR; //  CAN diagnosis register (reset: 0x0C)
__at(0x5427) volatile uint8_t CAN_FPSR; //  CAN page selection register (reset: 0x00)
__at(0x5428) volatile uint8_t CAN_P0; //  CAN paged register 0 (reset: 0xXX)
__at(0x5429) volatile uint8_t CAN_P1; //  CAN paged register 1 (reset: 0xXX)
__at(0x542A) volatile uint8_t CAN_P2; //  CAN paged register 2 (reset: 0xXX)
__at(0x542B) volatile uint8_t CAN_P3; //  CAN paged register 3 (reset: 0xXX)
__at(0x542C) volatile uint8_t CAN_P4; //  CAN paged register 4 (reset: 0xXX)
__at(0x542D) volatile uint8_t CAN_P5; //  CAN paged register 5 (reset: 0xXX)
__at(0x542E) volatile uint8_t CAN_P6; //  CAN paged register 6 (reset: 0xXX)
__at(0x542F) volatile uint8_t CAN_P7; //  CAN paged register 7  (reset: 0xXX)
__at(0x5430) volatile uint8_t CAN_P8; //  CAN paged register 8 (reset: 0xXX)
__at(0x5431) volatile uint8_t CAN_P9; //  CAN paged register 9 (reset: 0xXX)
__at(0x5432) volatile uint8_t CAN_PA; //  CAN paged register A (reset: 0xXX)
__at(0x5433) volatile uint8_t CAN_PB; //  CAN paged register B (reset: 0xXX)
__at(0x5434) volatile uint8_t CAN_PC; //  CAN paged register C (reset: 0xXX)
__at(0x5435) volatile uint8_t CAN_PD; //  CAN paged register D (reset: 0xXX)
__at(0x5436) volatile uint8_t CAN_PE; //  CAN paged register E (reset: 0xXX)
__at(0x5437) volatile uint8_t CAN_PF; //  CAN paged register F (reset: 0xXX)

#define PA_BASE 0x5000
#define PB_BASE 0x5005
#define PC_BASE 0x500A
#define PD_BASE 0x500F
#define PE_BASE 0x5014
#define PF_BASE 0x5019
#define PG_BASE 0x501E
#define PH_BASE 0x5023
#define PI_BASE 0x5028

#define Px_ODR 0
#define Px_IDR 1
#define Px_DDR 2
#define Px_CR1 3
#define Px_CR2 4

__at(PA_BASE + Px_ODR) volatile uint8_t PA_ODR;
__at(PA_BASE + Px_IDR) volatile uint8_t PA_IDR;
__at(PA_BASE + Px_DDR) volatile uint8_t PA_DDR;
__at(PA_BASE + Px_CR1) volatile uint8_t PA_CR1;
__at(PA_BASE + Px_CR2) volatile uint8_t PA_CR2;

__at(PB_BASE + Px_ODR) volatile uint8_t PB_ODR;
__at(PB_BASE + Px_IDR) volatile uint8_t PB_IDR;
__at(PB_BASE + Px_DDR) volatile uint8_t PB_DDR;
__at(PB_BASE + Px_CR1) volatile uint8_t PB_CR1;
__at(PB_BASE + Px_CR2) volatile uint8_t PB_CR2;

__at(PC_BASE + Px_ODR) volatile uint8_t PC_ODR;
__at(PC_BASE + Px_IDR) volatile uint8_t PC_IDR;
__at(PC_BASE + Px_DDR) volatile uint8_t PC_DDR;
__at(PC_BASE + Px_CR1) volatile uint8_t PC_CR1;
__at(PC_BASE + Px_CR2) volatile uint8_t PC_CR2;

__at(PD_BASE + Px_ODR) volatile uint8_t PD_ODR;
__at(PD_BASE + Px_IDR) volatile uint8_t PD_IDR;
__at(PD_BASE + Px_DDR) volatile uint8_t PD_DDR;
__at(PD_BASE + Px_CR1) volatile uint8_t PD_CR1;
__at(PD_BASE + Px_CR2) volatile uint8_t PD_CR2;

__at(PE_BASE + Px_ODR) volatile uint8_t PE_ODR;
__at(PE_BASE + Px_IDR) volatile uint8_t PE_IDR;
__at(PE_BASE + Px_DDR) volatile uint8_t PE_DDR;
__at(PE_BASE + Px_CR1) volatile uint8_t PE_CR1;
__at(PE_BASE + Px_CR2) volatile uint8_t PE_CR2;

__at(PF_BASE + Px_ODR) volatile uint8_t PF_ODR;
__at(PF_BASE + Px_IDR) volatile uint8_t PF_IDR;
__at(PF_BASE + Px_DDR) volatile uint8_t PF_DDR;
__at(PF_BASE + Px_CR1) volatile uint8_t PF_CR1;
__at(PF_BASE + Px_CR2) volatile uint8_t PF_CR2;

__at(PG_BASE + Px_ODR) volatile uint8_t PG_ODR;
__at(PG_BASE + Px_IDR) volatile uint8_t PG_IDR;
__at(PG_BASE + Px_DDR) volatile uint8_t PG_DDR;
__at(PG_BASE + Px_CR1) volatile uint8_t PG_CR1;
__at(PG_BASE + Px_CR2) volatile uint8_t PG_CR2;

__at(PH_BASE + Px_ODR) volatile uint8_t PH_ODR;
__at(PH_BASE + Px_IDR) volatile uint8_t PH_IDR;
__at(PH_BASE + Px_DDR) volatile uint8_t PH_DDR;
__at(PH_BASE + Px_CR1) volatile uint8_t PH_CR1;
__at(PH_BASE + Px_CR2) volatile uint8_t PH_CR2;

__at(PI_BASE + Px_ODR) volatile uint8_t PI_ODR;
__at(PI_BASE + Px_IDR) volatile uint8_t PI_IDR;
__at(PI_BASE + Px_DDR) volatile uint8_t PI_DDR;
__at(PI_BASE + Px_CR1) volatile uint8_t PI_CR1;
__at(PI_BASE + Px_CR2) volatile uint8_t PI_CR2;

/* Interrupts */
#define TLI_ISR                 0
#define AWU_ISR                 1
#define CLK_ISR                 2
#define EXTI0_ISR               3
#define EXTI1_ISR               4
#define EXTI2_ISR               5
#define EXTI3_ISR               6
#define EXTI4_ISR               7
#define BECAN_RX_ISR            8
#define BECAN_TX_ISR            9
#define SPI_ISR                 10
#define TIM1_OVF_ISR            11
#define TIM1_CC_ISR             12
#define TIM2_OVF_ISR            13
#define TIM2_CC_ISR             14
#define TIM3_OVF_ISR            15
#define TIM3_CC_ISR             16
#define UART1_TXC_ISR           17
#define UART1_RXC_ISR           18
#define I2C_ISR                 19
#define UART2_TXC_ISR           20
#define UART2_RXC_ISR           21
#define ADC1_ISR                22
#define TIM4_ISR                23
#define FLASH_ISR               24

#define enableInterrupts()    __asm__("rim");  /* enable interrupts */
#define disableInterrupts()   __asm__("sim");  /* disable interrupts */
#define waitForInterrupt()    __asm__("wfi");  /* wait for interrupt */

#define BITSET(port, bit) port |= (1u << bit)
#define BITCLR(port, bit) port &= ~(1u << bit)
#define BITFLIP(port, bit) port ^= (1u << bit)

#endif // STM8_H
