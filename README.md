# SPI NAND

Minimal TMS320F280039C firmware project for developing an SPI NAND driver.

## Layering

- `BSW/DMCU`: MCU clock and interrupt initialization.
- `BSW/DGPIO`: GPIO configuration and access.
- `BSW/DSPI`: blocking, full-duplex SPI transactions.
- `BRD`: concrete MCU, GPIO, SPI, and SPI NAND configuration.
- `MSW/SPI_NAND`: vendor-neutral SPI NAND commands.
- `ASW/MAIN`: board startup and a non-destructive `READ ID` bring-up test.

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

`BRD/BRD_SPI_NAND.c` contains the device-specific defaults:

- one `0x00` dummy/address byte before `READ ID` data;
- status register busy mask `0x01`;
- a bounded ready-poll count.

Confirm these values against the selected NAND datasheet. The MSW template
already provides reset, ID, feature, page-read, cache-read, program, and erase
command building blocks. Program and erase calls intentionally do not issue
`WRITE ENABLE` automatically.

SPI data buffers use `Uint16_t` elements because of the C2000 data model; only
the low eight bits of each element are transferred.

## First Bring-Up

Build and run the `CPU1_FLASH` configuration in Code Composer Studio. Inspect:

- `SPI_NAND_applicationStatus`
- `SPI_NAND_deviceId`

The startup test only initializes the hardware and reads three ID bytes.
