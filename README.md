# SPI NAND Flash

Winbond W25N01GVSFIG driver for the TI TMS320F280039C.
Provides initialization, identification, page reads/programming, block erase,
and ECC/bad-block status handling. Hardware validation is still required.

The application initializes the board and NAND, then handles five serial commands
in one module. The [Python application folder](host/README.md) contains guidance;
the client library and CLI are not yet implemented.

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

Serial defaults: SCI-A at 115200 baud, 8N1, RX GPIO28 and TX GPIO29. Pins are
configured in `BRD_GPIO.h`; serial and timer settings are in
[BRD_SCI.c](BRD/BRD_SCI.c). The transport owns CPU timer 0 at 1 MHz.
Confirm routing on the actual board before connecting.

## Serial commands

`INFO`, `READ_PAGE`, `PROGRAM_PAGE`, `ERASE_BLOCK`, and `IS_BAD_BLOCK` use physical
addresses. [APP_IF](ASW/APP_IF/APP_IF.c) handles framing, CRC checks, dispatch,
and responses. See [the protocol](ASW/APP_IF/PROTOCOL.md) for payloads, statuses,
and recovery. Python will own file handling, page splitting, bad-block allocation,
and read-back verification.

## Integration

Peripheral drivers live in `BSW/`, board configuration in `BRD/`, the NAND
driver in `MSW/`, and application code in `ASW/`.
See [W25N01GVSFIG.h](MSW/W25N01GVSFIG/W25N01GVSFIG.h) for the API.

- Buffers use `Uint16_t` elements; only the low eight bits are transferred.
- Programs require 512-byte aligned offsets and lengths. Callers must program
  pages in ascending order and limit each page to four programs between erases.
- `ECC_CORRECTED` means valid data; uncorrectable data is not copied to the caller.
- Bad-block replacement and power-loss recovery remain application responsibilities.

## Validation

Run from this directory:

```text
python -m unittest discover -s tests -v
```

Tests compile the C application against mocked NAND operations using a native
C compiler (GCC/Clang or Visual Studio C tools). They cover frame integrity,
dispatch, payload bounds, driver results, and interrupted-message recovery.
Serial timing, pin routing, and NAND operations still require board validation.
