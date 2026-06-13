# W25N01GVSFIG SPI NAND

Minimal TMS320F280039C firmware project for developing a Winbond
W25N01GVSFIG SPI NAND driver.

## Layering

- `BSW/DMCU`: MCU clock and interrupt initialization.
- `BSW/DGPIO`: GPIO configuration and access.
- `BSW/DSPI`: blocking, full-duplex SPI transactions.
- `BRD`: concrete MCU, GPIO, SPI, and W25N01GVSFIG configuration.
- `MSW/W25N01GVSFIG`: device-specific driver template.
- `ASW/MAIN`: board startup and an idle application loop.

## Default Wiring

| Signal | MCU pin |
| --- | --- |
| SPIA SIMO | GPIO16 |
| SPIA SOMI | GPIO17 |
| SPIA CLK | GPIO18 |
| NAND CS# | GPIO19 |

Change the pin definitions in `BRD/BRD_GPIO.h` to match the target hardware.
The SPI clock defaults to 10 MHz in `BRD/BRD_SPI.c`.

## Device Configuration

`BRD/BRD_W25N01GVSFIG.c` owns the device instance and board configuration:

- a bounded ready-poll count.
- the SPI handle.
- the chip-select GPIO.

The MSW driver provides initialization, JEDEC identification, page reads,
page programming, block erasing, and factory bad-block checks. Initialization
enables internal ECC and buffer-read mode. The board configuration explicitly
chooses whether initialization removes the device's volatile block protection.

SPI data buffers use `Uint16_t` elements because of the C2000 data model; only
the low eight bits of each element are transferred.

## Device Notes

- 2,048 data bytes plus 64 spare bytes per page.
- 64 pages per 128 KiB erase block.
- 1,024 blocks and 65,536 pages.
- JEDEC ID `EF AA 21`.
- Internal ECC and factory bad-block handling are required parts of the design.

## Driver Rules

- `readPage` and `programPage` accept a page number and an offset within the
  2,048-byte data area.
- `programPage` requires 512-byte aligned sector data while internal ECC is
  enabled and rejects attempts to change a programmed `0` bit back to `1`.
- `eraseBlock` erases one complete 128 KiB block.
- A corrected read returns `W25N01GVSFIG_STATUS_ECC_CORRECTED`; its data is
  valid. Uncorrectable data is not copied to the caller.
- Program and erase operations reject blocks with a non-`0xFF` spare-area bad
  block marker.
- The application must program pages in ascending order within a block and
  must not exceed four partial program operations per page between erases.
