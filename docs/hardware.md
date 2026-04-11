# PetDisk Hardware Documentation

## Overview
PetDisk is an IEEE-488 to SD card interface for the Commodore PET computer.
It emulates a CBM 2031 disk drive (device address 8) using a micro SD card as storage.

## Power Supply
Power is derived from the Commodore PET's second cassette tape connector (User Port side):
- Pin 5: +5 V
- Pin 1: GND

The STM8S105K6T6C runs at 5V (VDD = 5V).
The SD card requires 3.3V logic and power.
- Logic level shifting: 1 kΩ / 2 kΩ resistor voltage dividers on SCK, MOSI, and CS lines
  (5V → [1 kΩ] → SD pin → [2 kΩ] → GND gives ≈ 3.33 V)
- MISO: 3.3 V output from SD card is safe to connect directly to STM8 input (logic HIGH threshold ≈ 1.4 V)
- SD card VCC: AMS1117-3.3 LDO regulator from 5V

## STM8S105K6T6C Pin Assignments

| Pin | Net        | Direction    | Description                          |
|-----|-----------|-------------|--------------------------------------|
| PB0 | DIO1      | Open-drain   | IEEE-488 data bit 1 (active LOW)     |
| PB1 | DIO2      | Open-drain   | IEEE-488 data bit 2                  |
| PB2 | DIO3      | Open-drain   | IEEE-488 data bit 3                  |
| PB3 | DIO4      | Open-drain   | IEEE-488 data bit 4                  |
| PB4 | DIO5      | Open-drain   | IEEE-488 data bit 5                  |
| PB5 | DIO6      | Open-drain   | IEEE-488 data bit 6                  |
| PB6 | DIO7      | Open-drain   | IEEE-488 data bit 7                  |
| PB7 | DIO8      | Open-drain   | IEEE-488 data bit 8                  |
| PD0 | DAV       | Open-drain   | IEEE-488 Data Valid (active LOW)     |
| PD1 | NRFD      | Open-drain   | IEEE-488 Not Ready For Data          |
| PD2 | NDAC      | Open-drain   | IEEE-488 Not Data Accepted           |
| PD3 | ATN       | Input        | IEEE-488 Attention (active LOW)      |
| PD4 | EOI       | Open-drain   | IEEE-488 End Or Identify             |
| PD5 | SRQ       | Open-drain   | IEEE-488 Service Request             |
| PD6 | IFC       | Input        | IEEE-488 Interface Clear             |
| PD7 | REN       | Input        | IEEE-488 Remote Enable               |
| PC5 | SPI1_SCK  | Output       | SD card SPI clock (via divider)      |
| PC6 | SPI1_MOSI | Output       | SD card SPI data out (via divider)   |
| PC7 | SPI1_MISO | Input        | SD card SPI data in (direct)         |
| PA3 | SPI1_NSS  | Output       | SD card chip select (via divider)    |

## IEEE-488 Connector (J1 – 24-pin Amphenol 57-30240)
Standard GPIB pinout:

| Pin | Signal | Pin | Signal |
|-----|--------|-----|--------|
|  1  | DIO1   | 13  | DIO5   |
|  2  | DIO2   | 14  | DIO6   |
|  3  | DIO3   | 15  | DIO7   |
|  4  | DIO4   | 16  | DIO8   |
|  5  | EOI    | 17  | REN    |
|  6  | DAV    | 18  | GND    |
|  7  | NRFD   | 19  | GND    |
|  8  | NDAC   | 20  | GND    |
|  9  | IFC    | 21  | GND    |
| 10  | SRQ    | 22  | GND    |
| 11  | ATN    | 23  | GND    |
| 12  | SHIELD | 24  | GND    |

## Voltage Divider Network
Each 5V output to 3.3V SD card input uses:
- R_top = 1 kΩ (between MCU output and SD pin)
- R_bot = 2 kΩ (between SD pin and GND)

Vout = 5V × 2000 / (1000 + 2000) = 3.33 V ✓

Three divider pairs are needed:
- R1/R2: SCK  (PC5)
- R3/R4: MOSI (PC6)
- R5/R6: CS   (PA3)

## Bypass Capacitors
- C1–C3: 100 nF ceramic, placed close to VDD pins of U1
- C4: 10 µF electrolytic, on 5V rail near J3

## Bill of Materials (Abbreviated)

| Ref | Value     | Description                        |
|-----|-----------|------------------------------------|
| U1  | STM8S105K6T6C | 32-pin LQFP STM8 MCU           |
| U2  | AMS1117-3.3   | 3.3V 1A LDO regulator           |
| J1  | GPIB-24       | 24-pin IEEE-488 connector        |
| J2  | microSD       | Micro SD socket (push-push)      |
| J3  | TAPE-5        | PET tape connector (5-pin)       |
| R1,R3,R5 | 1 kΩ | Voltage divider top resistors   |
| R2,R4,R6 | 2 kΩ | Voltage divider bottom resistors|
| C1–C3 | 100 nF  | MCU VDD bypass capacitors        |
| C4  | 10 µF     | Bulk supply capacitor            |

## Firmware
See `firmware/` directory. The firmware is written in C++23 and targets the STM8S with SDCC,
or Linux with GCC for testing.

Build tests (requires CMake ≥ 3.20 and GCC ≥ 13):
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```
