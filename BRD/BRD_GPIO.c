/*
|===============================================================================
|
| File:         BRD_GPIO.c
|
| Project:      SPI_NAND
|
| Processor:    TI TMS320F28003x
|
| Description:  GPIO configuration for the SPI NAND interface.
|
|=============================================================================*/

/*=== INCLUDE FILES ==========================================================*/

#include "BRD_GPIO.h"

/*=== GLOBAL DATA ============================================================*/

const GpioConfig_t BRD_GPIO_CONFIG[GPIO_INSTANCE_COUNT] =
{
    [GPIO_INSTANCE_SPIA_SIMO] =
    {
        BRD_GPIO_SPIA_SIMO_GPIO,
        BRD_GPIO_SPIA_SIMO,
        GPIO_DIR_UNDEFINED,
        GPIO_QUAL_ASYNC,
        GPIO_PIN_TYPE_STD,
        GPIO_STATE_DONT_CARE,
        FALSE,
        FALSE
    },
    [GPIO_INSTANCE_SPIA_SOMI] =
    {
        BRD_GPIO_SPIA_SOMI_GPIO,
        BRD_GPIO_SPIA_SOMI,
        GPIO_DIR_UNDEFINED,
        GPIO_QUAL_ASYNC,
        GPIO_PIN_TYPE_PULLUP,
        GPIO_STATE_DONT_CARE,
        FALSE,
        FALSE
    },
    [GPIO_INSTANCE_SPIA_CLK] =
    {
        BRD_GPIO_SPIA_CLK_GPIO,
        BRD_GPIO_SPIA_CLK,
        GPIO_DIR_UNDEFINED,
        GPIO_QUAL_ASYNC,
        GPIO_PIN_TYPE_STD,
        GPIO_STATE_DONT_CARE,
        FALSE,
        FALSE
    },
    [GPIO_INSTANCE_SPI_NAND_CS] =
    {
        BRD_GPIO_SPI_NAND_CS_GPIO,
        BRD_GPIO_SPI_NAND_CS,
        GPIO_DIR_OUT,
        GPIO_QUAL_SYNC,
        GPIO_PIN_TYPE_PULLUP,
        GPIO_STATE_ON,
        FALSE,
        FALSE
    }
};

/*=== End of File ============================================================*/
