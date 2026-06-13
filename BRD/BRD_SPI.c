/*
|===============================================================================
|
| File:         BRD_SPI.c
|
| Project:      SPI_NAND
|
| Processor:    TI TMS320F28003x
|
| Description:  SPI peripheral configuration.
|
|=============================================================================*/

/*=== INCLUDE FILES ==========================================================*/

#include "BRD_SPI.h"
#include "BRD_GPIO.h"
#include "DMCU.h"

/*=== GLOBAL DATA ============================================================*/

const SpiConfig_t BRD_SPI_CONFIG =
{
    SPIA_BASE,
    MCU_LSPCLK_FREQ,
    10000000uL,
    8u,
    SPI_PROT_POL0PHA0,
    SPI_MODE_CONTROLLER,
    &BRD_GPIO_CONFIG[GPIO_INSTANCE_SPIA_SIMO],
    &BRD_GPIO_CONFIG[GPIO_INSTANCE_SPIA_SOMI],
    &BRD_GPIO_CONFIG[GPIO_INSTANCE_SPIA_CLK]
};

SpiHandle_t BRD_SPI_HANDLE;

/*=== End of File ============================================================*/
