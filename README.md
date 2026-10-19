# SPI NAND Flash

Winbond W25N01GVSFIG driver for the TI TMS320F280039C.
Provides initialization, identification, page reads/programming, block erase,
and ECC/bad-block status handling. Hardware validation is still required.

The application currently initializes the board and idles; it does not initialize
the NAND. A host command service and Python library/CLI are planned.

## Build

Import the `SPI_NAND` project into TI Code Composer Studio with C2000Ware and
the TI C2000 compiler installed. Build `CPU1_FLASH` and use the matching
configuration in `targetConfigs/` to debug or flash.

The optional [Justfile](Justfile) uses CCS-generated build files. Adjust its
local tool paths before using `just build` or `just flash` (requires UniFlash).

## Hardware

Default SPIA wiring: SIMO GPIO16, SOMI GPIO17, CLK GPIO18, CS# GPIO19.
Set pins in [BRD_GPIO.h](BRD/BRD_GPIO.h), SPI settings in
[BRD_SPI.c](BRD/BRD_SPI.c), and NAND configuration in
[BRD_W25N01GVSFIG.c](BRD/BRD_W25N01GVSFIG.c).

## Integration

Peripheral drivers live in `BSW/`, board configuration in `BRD/`, the NAND
driver in `MSW/`, and application code in `ASW/`.
See [W25N01GVSFIG.h](MSW/W25N01GVSFIG/W25N01GVSFIG.h) for the API.

- Buffers use `Uint16_t` elements; only the low eight bits are transferred.
- Programs require 512-byte aligned offsets and lengths. Callers must program
  pages in ascending order and limit each page to four programs between erases.
- `ECC_CORRECTED` means valid data; uncorrectable data is not copied to the caller.
- Bad-block replacement and power-loss recovery remain application responsibilities.
