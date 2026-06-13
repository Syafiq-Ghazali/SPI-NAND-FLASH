/*
|===============================================================================
|
| File:         BRD_W25N01GVSFIG.c
|
| Project:      SPI_NAND
|
| Processor:    TI TMS320F28003x
|
| Description:  Board configuration for the W25N01GVSFIG MSW module.
|
|=============================================================================*/

/*=== INCLUDE FILES ==========================================================*/

#include "BRD_W25N01GVSFIG.h"
#include "BRD_GPIO.h"
#include "BRD_SPI.h"

/*=== GLOBAL DATA ============================================================*/

const W25N01GVSFIGConfig_t BRD_W25N01GVSFIG_CONFIG =
{
    &BRD_SPI_HANDLE,
    BRD_GPIO_W25N01GVSFIG_CS_GPIO,
    10000uL,
    TRUE
};

W25N01GVSFIGHandle_t BRD_W25N01GVSFIG_HANDLE;

/*=== End of File ============================================================*/
