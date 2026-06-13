# Introduction {#mainpage} #

A Commodore PET to microSD card interface.

## Problem overview

I bought my first computer, a Commodore PET in 1977 and I still have it and [recently restored](https://github.com/beroset/pet_resto) it.  The PET used audio cassette tapes for program and data storage, and while that works, more or less, there's not a convenient way to use a modern computer to write cassette tapes in the PET's format.

So, the idea for this project is to connect the PET to a microSD card.  This would allow me to use high capacity modern storage media.  Indeed, a 64G microSD card would probably be able to store all the software ever written for the PET, so this seems adequate.

### Previous work
Doing some searching for the PET, I encountered two other closely related projects:

https://github.com/bitfixer/petdisk

https://github.com/bitfixer/petdisk-max

These both seem quite usable.  The first uses an Arduino, and the second one uses an ESP32.  One nice feature implemented is the ability to use a connection to a computer via WiFi as a simulated disk.

There are also commercially available devices, including from [Tynemouth Software](https://www.tindie.com/stores/tynemouth/) in the U.K. who make all manner of interesting things for old computers.

This project, however, is primarily intended to:

 - provide an opportunity to learn about the STM8 processor
 - provide an opportunity to learn about microSD card interfacing
 - build something fun
 - describe the process by which such things are described so that others might be inspired

## Process
The general process by which one creates a microprocessor based embedded system can be summarized in these steps:

 1. Identify interfaces
 2. Choose microprocessor
 3. Assign interfaces to microprocessor pins
 4. Identify other needed electronics & design the circuit
 5. Build the circuit
 6. Write the software
 7. Test

The rest of this document uses these steps as the outline.

## Identify interfaces

There are two major interfaces.  They are the IEEE-488 interface from the PET, and the microSD card.  In addition, it is often helpful to have a UART (serial port) available for debugging and an LED for status indications.

## Choose microprocessor
Previous work, as mentioned above, used an AVR microprocessor, and an ESP32.  This project will use an STMicroelectronics STM8S105K6T6C processors. It's a 16MHz 8-bit processor that with Harvard architecture and 3-stage pipeline in a 32-pin LQFP package. It feature 32K of FLASH, 2K of RAM, 1K of EEPROM, SPI, I2C, UART and several timers.  It can also operate at voltages from 2.95V to 5.5V.  Very usefully for this project, it has an internal RC oscillator, so very few external parts are needed, and the cost per CPU is less than $1.

## Assign interfaces to microprocessor pins
In the diagram below, the IEEE-488 port connections to the PET are shown on a diagram of the STM8S pinout.  Color codes are shown in the table.

Interface | color
----------|---------
IEEE-488  | cyan
microSD   | dark blue
ground    | orange
+5V       | light red
serial    | light green
LED       | yellow

@image html pin_assignments.png "Pin assignments"
@image latex pin_assignments.png "Pin assignments"

Here are the pin assignments in table format:

Pin | Function                                | Description      |
----|-----------------------------------------|------------------|
 1  | NRST                                    | 100nF to GND     |
 2  | OSCIN/PA1                               | IEEE-488 DAV     |
 3  | OSCOUT/PA2                              | IEEE-488 NRFD    |
 4  | VSS                                     | GND              |
 5  | VCAP                                    | 2.2uF to GND     |
 6  | VDD                                     | +5V              |
 7  | VDDIO                                   | +5V              |
 8  | AIN12/PF4                               | IEEE-488 NDAC    |
 9  | VDDA                                    | +5V              |
 10 | VSSA                                    | GND              |
 11 | [I2C_SDA] AIN5/PB5                      | IEEE-488 ATN     |
 12 | [I2C_SCL] AIN4/PB4                      | IEEE-488 EOI     |
 13 | [TIM1_ETR] AIN3/PB3                     | no connection    |
 14 | [TIM1_CH3N] AIN2/PB2                    | IEEE-488 DIO6    |
 15 | [TIM1_CH2N] AIN1/PB1                    | IEEE-488 DIO7    |
 16 | [TIM1_CH1N] AIN0/PB0                    | IEEE-488 DIO8    |
 17 | PE5/SPI_NSS                             | 2.2K ohm + LED   |
 18 | PC1 (HS)/TIM1_CH1/UART2_CK              | no-connection    |
 19 | PC2 (HS)/TIM1_CH2                       | no-connection    |
 20 | PC3 (HS)/TIM1_CH3                       | no-connection    |
 21 | PC4 (HS)/TIM1_CH4                       | SD card CS       |
 22 | PC5 (HS)/SPI_SCK                        | SD card CLK      |
 23 | PC6 (HS)/SPI_MOSI                       | SD card MOSI     |
 24 | PC7 (HS)/SPI_MISO                       | SD card MISO     |
 25 | PD0 (HS)/TIM3_CH2 [TIM1_BKIN] [CLK_CCO] | IEEE-488 DIO1    |
 26 | PD1 (HS)/SWIM                           | IEEE-488 DIO2    |
 27 | PD2 (HS)/TIM3_CH1 [TIM2_CH3]            | IEEE-488 DIO3    |
 28 | PD3 (HS)/TIM2_CH2 [ADC_ETR]             | IEEE-488 DIO4    |
 29 | PD4 (HS)/TIM2_CH1 [BEEP]                | IEEE-488 DIO5    |
 30 | PD5/UART2_TX                            | serial TX        |
 31 | PD6/UART2_RX                            | serial RX        |
 32 | PD7/TLI [TIM1_CH4]                      | no-connection    |


## Identify other needed electronics & design the circuit
For this project, since the IEEE-488 bus runs with 5V logic, but a microSD card runs on 3.3V logic, level translation is required.  For this purpose a modern, high-speed level converter such as the SN74LVC245APW is a good choice.  It will also require a 3.3V power supply, which can be very simply constructed using a LM1117-3.3 to provide 3.3V from 5V input.

In addition, we will need a 500nF decoupling capacitor near the power rails of the CPU, a 100nF capacitor for the NRST line to ground, and a 2.2uF capacitor connecting VCAP on pin 5 to ground.

Finally, the circuit needs a LED and current limiting resistor. The particular red LED chosen here has a forward voltage drop of 2.0V and an open drain output of this processor can easily safely sink 10mA, so we use Ohm's law to calculate an appropriate resistor.  \f(V/I = R\f) so calculate \f((5V - 2.0V)/10mA = 300\f) ohms, and the closest standard resistor value is 270 ohms, which gives a resulting current of 11mA.

TODO: add a proper schematic

## Build the circuit
For getting the circuit up and running quickly, a breadboard is useful.  An adapter board was used to allow the use of the LQFP package to the breadboard.  Additionally, a microSD module incorporating both the 3.3V power supply and level converters provides a convenient package for the circuitry that is needed to connect a microSD card to a 5V CPU.

@image html circuit.png "Completed circuit on breadboard"
@image latex circuit.png "Completed circuit on breadboard"

## Write the software
The details of the software are described in most of this manual; this section describes the process of creating the software.

Generically, the first consideration is tools, including compilers, linkers, debuggers and whatever else might be needed to create embedded software.  For this project and this CPU, the chosen tools are [SDCC](https://sdcc.sourceforge.net/) which is the Small Device C Compiler.  It's an open source compiler suite that has support for the STM8 processor used in this project.

To program the part while it is in-circuit, one can use the open source [stm8flash](https://github.com/vdudouyt/stm8flash) tool with an ST-Link programming device, which is an inexpensive piece of hardware (inexpensive clones are available under $10) that connects a computer to the circuit for programming and debugging.

Creating software for embedded systems is different from creating software for a typical desktop computer.  The first obvious difference is that developing software for a desktop machine usually means also developing that software on the same full-featured machine which already has an operating system, keyboard, mouse, and display.

Embedded systems often start with none of those, so cross-development is typically used.  That is, one develops the embedded software *on* a regular computer, but *for* the embedded system.

### First program
The first thing the is often done is to get some simple thing working first.  For many embedded systems, blinking an LED is that first task.  This has several benefits, including that:

 - it's simple
 - it provides a test that the toolchain works and is configured correctly
 - it gives immediate visual feedback that the CPU is working
 - it requires minimal external circuitry

### Next software steps, generically
Often the next useful thing to do is to enable the UART so that we can connect that to the development computer and conveniently print debugging messages or allow for simple serial requests.

## Test
It's most often useful to start with small subsystems, such as blinking LED and serial port, and then move on to more complex ones.

# Further reading
A more detailed overview of this project's architecture is describe in [architecture](@ref architecture)
