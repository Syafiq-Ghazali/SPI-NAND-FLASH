/*
|===============================================================================
|
| File:         MAIN.c
|
| Project:      SPI_NAND
|
| Processor:    TI TMS320F28003x
|
| Description:  SPI NAND firmware entry point.
|
|=============================================================================*/

/*=== INCLUDE FILES ==========================================================*/

#include "BRD.h"
#include "SPI_NAND.h"

/*=== GLOBAL DATA ============================================================*/

volatile SpiNandStatus_t SPI_NAND_applicationStatus;
Uint16_t SPI_NAND_deviceId[3];

/*=== FUNCTION DEFINITIONS ===================================================*/

void main
(
    void
)
{
    BRD_init();

    SPI_NAND_applicationStatus =
        SPI_NAND_init(&BRD_SPI_NAND_HANDLE, &BRD_SPI_NAND_CONFIG);

    if (SPI_NAND_applicationStatus == SPI_NAND_STATUS_OK)
    {
        SPI_NAND_applicationStatus =
            SPI_NAND_readId(&BRD_SPI_NAND_HANDLE,
                            SPI_NAND_deviceId,
                            (Uint32_t)(sizeof(SPI_NAND_deviceId) /
                                      sizeof(SPI_NAND_deviceId[0])));
    }

    for (;;)
    {
        /* Inspect SPI_NAND_applicationStatus and SPI_NAND_deviceId in CCS. */
    }
}

/*=== End of File ============================================================*/
