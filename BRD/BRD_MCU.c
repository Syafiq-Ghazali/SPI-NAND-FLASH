/*
|===============================================================================
|
| File:         BRD_MCU.c
|
| Project:      SPI_NAND
|
| Processor:    TI TMS320F28003x
|
| Description:  MCU clock and peripheral configuration.
|
|=============================================================================*/

/*=== INCLUDE FILES ==========================================================*/

#include "BRD_MCU.h"

/*=== PRIVATE DATA ===========================================================*/

static const SysCtl_PeripheralPCLOCKCR BRD_MCU_CLOCKS[] =
{
    SYSCTL_PERIPH_CLK_SPIA
};

/*=== GLOBAL DATA ============================================================*/

const McuConfig_t BRD_MCU_CONFIG =
{
    SYSCTL_LSPCLK_PRESCALE_2,
    BRD_MCU_CLOCKS,
    (Uint16_t)(sizeof(BRD_MCU_CLOCKS) / sizeof(BRD_MCU_CLOCKS[0])),
    FLASH0CTRL_BASE,
    FLASH0ECC_BASE,
    5u
};

/*=== End of File ============================================================*/
