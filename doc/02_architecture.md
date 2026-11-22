# Software Architecture {#architecture}

This page describes the software architecture of the project.

## Overview
As mentioned in the [introduction](@ref mainpage) page, there are two main hardware interfaces: the [IEEE-488 bus](@ref gpib) and the [microSD card](@ref microsd).  This project also uses the UART to communicate with the user for troubleshooting and testing.

### Separation of concerns
To have a clean architecture and to minimize linkage among files, we isolate implementation details of the various hardware interfaces within their associated files.

#### timer.c
This file implements timer and delay related items using timer3 hardware resources.

#### spi.c
The SPI bus implementation is in spi.c.

#### uart.c
The uart.c file contains hardware setup and interface for the UART which uses the hardware UART2.

#### diskio.c
This file contains low-level disk I/O for FatFs.

#### gpib.c
This file encapsulates the IEEE-488 (GPIB) details.

#### main.c
This file has the main loop which manages the state machine for the file system and also manages the UART to react to various single-letter commands.  Note that it also contains and owns the `sector_buf` buffer.

The *only* interaction that gpib.c module has with the buffer is via callbacks -- one for LOAD and one for SAVE.

## Buffers
The way the `sector_buf` buffer works for SAVE is that it has two contiguous half-buffers.  When the first half buffer is filled, the A_full flag is set and this is the signal to the main routine to write that half-buffer to the file.  While it's writing that half, the GPIB bus is continuing to fill the second half-buffer.  When that second half-buffer is filled, the B_full flag is set to indicate to the main loop to save that half.  When the SAVE is complete (signalled by `fstate` being `writing` but the GPIB `state` is `idle`), there may be some unwritten data in the buffer.  We can can tell which half by looking at the sector_buf_idx.

## Interrupts
As with many microcontroller projects, the code is primarily interrupt-driven.  After initialization, the `main` loop ends with the WFI (Wait For Interrupt) instruction.  The interrupt sources are:

 - timer2: used to toggle the blinking LED
 - timer3: used in timer routines
 - ATN: a GPIO interrupt for the IEEE-488 bus (B5)
 - DAV: a GPIO interrupt for the IEEE-488 bus (A2)

## IEEE-488 bus
The IEEE-488 bus implementation works with a state machine.  The device address is stored in the first byte of EEPROM.  Only PET `LOAD` and `SAVE` commands are currently supported, and these *must* include the file name, even though this is optional on the PET side.

### SAVE command

With program `55 REM`, and then executing `SAVE "TEST",8` on a PET with new ROMs, the data is as follows, with the first single digit representing the flags (ATN = 1, EOI = 2) and the following two hex digits representing the data on the bus:

```
1 28 1 f1 0 54 0 45 0 53 2 54 1 3f 1 28 1 61 0 01 0 04 0 07 0 04 0 37 0 00 0 8f 0 00 0 00 2 00 1 3f 1 28 1 e1 1 3f
```

Here is an interpretation of what that means:
```
1 28  LAG 08
1 f1  SCG 17  (SAVE)
0 54    T
0 45    E
0 53    S
2 54    T
1 3f   UNL
1 28  LAG 08
1 61  SCG 01
0 01   \
0 04   /  loc in memory (0x401)
0 07   \
0 04   /  next stmt (0x0407)
0 37     \
0 00     / line number (55)
0 8f   REM token
0 00   delimiter
0 00
2 00   eoi
1 3f   UNL
1 28  LAG 08
1 e1  SCG 01 CLOSE
1 3f   UNL
```

With the old ROM:
```
1 28 1 f1 0 54 0 45 0 53 2 54 1 3f 1 3f 1 00 1 07 1 04 1 37 1 00 1 8f 1 00 1 00 1 00 3 24 1 3f 1 3f 1 e1
```

```
1 28  LAG 08
1 f1  SCG 17  (SAVE)
0 54    T
0 45    E
0 53    S
2 54    T
1 3f   UNL
1 3f   UNL
1 00  delimiter
1 07   \
1 04   /  next stmt (0x0407)
1 37     \
1 00     / line number (55)
1 8f   REM token
1 00   delimiter
1 00
3 24   eoi  (0x24 is unused)
1 3f   UNL
1 3f   UNL
1 e1  SCG 01 CLOSE
```

We'll start with just the implementation for the new ROMs.  The second sent byte is either 0xF1 (SAVE) or 0xF0 (LOAD).  As noted earlier, the file name is required to be sent next, with the last byte of the file name marked with EOI.  After this, if the LOAD command was issued, the PET sends TAG 08 (0x48) or LAG 08 (0x28), assuming the device address is 8.

### LOAD command
Here's what `LOAD "TEST",8` looks like for new ROMs.  Note that because DAV and EOI are always driven by the Talker and NRFD and NDAC by the receiver, the roles for these lines will need to be adjusted after the "SCG 00" part in the transfer shown below, and then changed back after the last data byte is sent with EOI.  The ATN line is *always* controlled by the PET.

This assumes that the file named "TEST" is on the SD card and has the following contents:

```
0000: 01 04 07 04 37 00 8f 00 00 00
```
It's how the PET encodes an extremely short but valid BASIC program:

```
55 REM
```

```
1 28  LAG 08
1 f0  SCG 16  (LOAD)
0 54    T 
0 45    E
0 53    S
2 54    T
1 3f   UNL
1 48  TAG 08  Talker address
1 60  SCG 00
   ; STM8 changes from Listener to Talker
0 01   \
0 04    \
0 07     \
0 04      \
0 37
0 00        file contents
0 8f      /
0 00     /
0 00    /
2 00   / eoi
   ; STM8 change from Talker back to Listener
1 5f   UNT (untalk)
1 28  LAG 08
1 e0  SCG 00 CLOSE file
1 3f   UNL (unlisten)
```

### Talker vs. Listener

| Pin  | Listener Mode | Talker Mode |
|------|--------------|-------------|
| DAV  | IN (PET drives) | OUT (STM8 drives) |
| EOI  | IN (PET drives) | OUT (STM8 drives) |
| NRFD | OUT (STM8 drives) | IN (PET drives) |
| NDAC | OUT (STM8 drives) | IN (PET drives) |
