/*
|===============================================================================
|
| File:         BRD.c
|
| Project:      SPI_NAND
|
| Processor:    TI TMS320F28003x
|
| Description:  Initializes the hardware used by the SPI NAND project.
|
|=============================================================================*/

/*=== INCLUDE FILES ==========================================================*/

#include "BRD.h"

/*=== FUNCTION DEFINITIONS ===================================================*/

void BRD_init
(
    void
)
{
    DISABLE_GLOBAL_INTERRUPT();

    DMCU_init(&BRD_MCU_CONFIG);
    DGPIO_init(&BRD_GPIO_CONFIG[GPIO_INSTANCE_SPI_NAND_CS]);
    (void)DSPI_init(&BRD_SPI_HANDLE, &BRD_SPI_CONFIG);

    ENABLE_GLOBAL_INTERRUPT();
    ENABLE_GLOBAL_REALTIME_INTERRUPT_DBGM();
}

/*=== End of File ============================================================*/
