/*
|===============================================================================
|
| File:         BRD_SPI_NAND.c
|
| Project:      SPI_NAND
|
| Processor:    TI TMS320F28003x
|
| Description:  Board configuration for the SPI NAND MSW module.
|
|=============================================================================*/

/*=== INCLUDE FILES ==========================================================*/

#include "BRD_SPI_NAND.h"
#include "BRD_GPIO.h"
#include "BRD_SPI.h"

/*=== GLOBAL DATA ============================================================*/

const SpiNandConfig_t BRD_SPI_NAND_CONFIG =
{
    &BRD_SPI_HANDLE,
    BRD_GPIO_SPI_NAND_CS_GPIO,
    1u,
    0x00u,
    0x01u,
    10000uL
};

SpiNandHandle_t BRD_SPI_NAND_HANDLE;

/*=== End of File ============================================================*/
