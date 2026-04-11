# petdisk

PetDisk is an IEEE-488 to SD card interface for the Commodore PET computer.
It emulates a CBM 2031 floppy disk drive (device address 8) using a micro SD card as storage, bridged by an STM8S105K6T6C microcontroller.

## Hardware Summary

| Block | Description |
|-------|-------------|
| **MCU** | STM8S105K6T6C (LQFP-32, 5 V, 16 MHz) |
| **Bus** | IEEE-488 / GPIB, open-drain, active-LOW signals |
| **Storage** | Micro SD card via SPI (SPI Mode 0, 3.3 V logic) |
| **Power** | +5 V and GND from the Commodore PET second tape connector |
| **Level shift** | 1 kΩ / 2 kΩ resistor voltage dividers on SCK, MOSI, CS (5 V → ~3.33 V) |
| **SD VCC** | AMS1117-3.3 LDO regulator |

### Pin Assignments

| Pin | Net        | Function |
|-----|-----------|----------|
| PB0–PB7 | DIO1–DIO8 | IEEE-488 data bus (open-drain) |
| PD0 | DAV   | IEEE-488 Data Valid |
| PD1 | NRFD  | IEEE-488 Not Ready For Data |
| PD2 | NDAC  | IEEE-488 Not Data Accepted |
| PD3 | ATN   | IEEE-488 Attention (input) |
| PD4 | EOI   | IEEE-488 End Or Identify |
| PD5 | SRQ   | IEEE-488 Service Request |
| PD6 | IFC   | IEEE-488 Interface Clear (input) |
| PD7 | REN   | IEEE-488 Remote Enable (input) |
| PC5 | SPI1_SCK  | SD card SPI clock (via voltage divider) |
| PC6 | SPI1_MOSI | SD card SPI MOSI (via voltage divider) |
| PC7 | SPI1_MISO | SD card SPI MISO (direct 3.3 V input) |
| PA3 | SPI1_NSS  | SD card chip select (via voltage divider) |

See [`docs/hardware.md`](docs/hardware.md) for the full pin table, BOM, and voltage-divider details.

## Schematic

KiCad 7 schematic lives in [`schematic/`](schematic/).  Open `petdisk.kicad_sch` in KiCad 7 or later.

## Firmware

```
firmware/
├── include/        # Platform-independent headers (HAL, IEEE-488, SD card, CBM DOS)
├── src/            # C11 implementation + Linux HAL (C++) + STM8 HAL (C)
└── tests/          # Catch2 unit tests (C++)
```

### Modules

| Module | Description |
|--------|-------------|
| `hal.h` | C vtable interfaces: `IGpioPin`, `IGpioPort`, `ISpi`, `ITimer`, `IFilesystem` |
| `ieee488` | GPIB three-wire handshake, listen/talk addressing, ATN/IFC/EOI |
| `sdcard`  | SPI SD init (CMD0/CMD8/ACMD41/CMD58), 512-byte block R/W |
| `cbmdos`  | CBM DOS channels 0–15, command/status, filesystem DI |
| `hal_linux.cpp` | In-memory HAL for Linux simulation and testing |
| `hal_stm8.c`    | Register-level STM8 HAL (compiled with SDCC only) |

### Building (Linux / GCC ≥ 13)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

### Running tests

```bash
./build/firmware/tests/petdisk_tests
```

All 33 test cases (583 assertions) should pass.

### Cross-compiling for STM8 (SDCC)

The firmware protocol modules (`ieee488.c`, `sdcard.c`, `cbmdos.c`, `petdisk.c`)
are plain C11 and compile with SDCC.  The Linux HAL and Catch2 test harness
remain in C++ for host-side development and testing.

SDCC cannot link multiple source files in a single invocation (it raises
_warning 120_ and silently ignores the extra files).  Each source file must
be compiled to a `.rel` object separately, then linked with a second SDCC call.
The provided `Makefile` handles this automatically:

```bash
make              # produces petdisk.ihx in the current directory
make clean        # remove build artefacts
```

The Makefile defaults to `sdcc-sdcc` (the Fedora 43 package name).
Override `CC` on the command line if needed:

```bash
make CC=sdcc      # use plain sdcc
```

The STM8 entry point is `firmware/src/main_stm8.c`.  It includes a minimal
stub `IFilesystem` (returns FILE NOT FOUND for every operation) which should
be replaced with a real FAT filesystem driver once available.

## License

BSD 3-Clause – see [LICENSE](LICENSE).
