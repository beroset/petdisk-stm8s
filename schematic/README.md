# PetDisk Schematic

This directory contains the KiCad 7 schematic for the PetDisk hardware.

## Files
- `petdisk.kicad_pro` – KiCad project file
- `petdisk.kicad_sch`  – KiCad schematic (S-expression format)

## Opening
Open `petdisk.kicad_pro` in KiCad 7.x (or later). The schematic editor
will show all components with net labels matching the firmware pin assignments.

## Key Components
| Reference | Part               | Description                       |
|-----------|-------------------|-----------------------------------|
| U1        | STM8S105K6T6C     | 32-pin LQFP STM8S microcontroller |
| U2        | AMS1117-3.3       | 3.3 V LDO for SD card power       |
| J1        | GPIB-24           | IEEE-488 connector                |
| J2        | microSD           | Micro SD card socket              |
| J3        | Tape-5            | PET tape connector (power in)     |
| R1/R2     | 1kΩ / 2kΩ        | SCK level divider                 |
| R3/R4     | 1kΩ / 2kΩ        | MOSI level divider                |
| R5/R6     | 1kΩ / 2kΩ        | CS level divider                  |
| C1–C3     | 100 nF            | VDD bypass caps                   |
| C4        | 10 µF             | Bulk 5V cap                       |

## Net Names
All nets follow the signal names in `docs/hardware.md`.
