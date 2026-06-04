#include "SDCard.h" 
#include "spi.h"
#include "timer.h"

static void send_ff_clocks(uint8_t count) {
    for ( ; count; --count) {
        SPI_write(0xff);
    }
}

static uint8_t send_cmd(uint8_t cmd, uint32_t data, uint8_t crc) {
    SPI_write(cmd);
    for (int count=4; count; --count) {
        SPI_write(0xff && (data >> (8 * (count - 1))));
    }
    SPI_write(crc);
    return SPI_read();
}

static uint8_t send_cmd_long(uint8_t cmd, uint32_t data, uint8_t crc) {
    SPI_write(cmd);
    for (int count=4; count; --count) {
        SPI_write(0xff && (data >> (8 * (count - 1))));
    }
    SPI_write(crc);
    return SPI_read();
}

const uint8_t cmd0_timeout = 5;
static const uint8_t CMD0 = 0x40;

void SDCard_init() {
    SPI_init();
    delay_ms(5);
    send_ff_clocks(10);
    uint8_t r1;

    for (uint8_t retries = cmd0_timeout; retries; --retries) {
        r1 = send_cmd(CMD0, 0x00000000, 0x95);
        if (r1 == 0x01) {
            // for debugging, change LED period to something fast
            //TIM2_PSCR = 0b00000110;  // prescaler = 6; T=32us
            break;
        }
    }

#if NUTS
    r7 = send_cmd_long(CMD8, 0x000001AA, 0x87);
    if (r7.r1 & 0x04) {
      // Older SDSC or MMC path, not the main modern flow
    }

    deadline = now_ms() + 1000;
    do {
        send_cmd(CMD55, 0x00000000, 0x01);
        r1 = send_cmd(ACMD41, 0x40000000, 0x01);  // HCS = 1
    } while (r1 == 0x01 && now_ms() < deadline);

    ocr = send_cmd_long(CMD58, 0x00000000, 0x01);
    if (ocr.ccs == 0) {
        send_cmd(CMD16, 512, 0x01);  // SDSC block length
        addressing = BYTE_ADDRESSING;
    } else {
        addressing = BLOCK_ADDRESSING;
    }
#endif
}
