# Software Architecture {#archite
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
0 04   /  loc in memory (0x401)
0 07   \
0 04   /  next stmt (0x0407)
0 37     \
0 00     / line number (55)
0 8f   REM token
0 00   delimiter
0 00
2 00   eoi
   ; STM8 change from Talker back to Listener
1 5f   UNT (untalk)
1 28  LAG 08
1 e0  SCG 00 CLOSE file
1 3f   UNL (unlisten)
```
