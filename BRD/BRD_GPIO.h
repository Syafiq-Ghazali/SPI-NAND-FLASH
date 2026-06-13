/*
|===============================================================================
|
| File:         BRD_GPIO.h
|
| Project:      SPI_NAND
|
| Processor:    TI TMS320F28003x
|
| Description:  GPIO assignments for the SPI NAND interface.
|
|=============================================================================*/

#ifndef BRD_GPIO_H
#define BRD_GPIO_H

/*=== INCLUDE FILES ==========================================================*/

#include "DGPIO.h"

/*=== #DEFINES ===============================================================*/

/*
 * Default TMS320F280039C SPIA mapping. Update these four definitions if the
 * SPI NAND is connected to different pins on the target board.
 */
#define BRD_GPIO_SPIA_SIMO_GPIO          (16u)
#define BRD_GPIO_SPIA_SIMO               ((Uint32_t)GPIO_16_SPIA_SIMO)

#define BRD_GPIO_SPIA_SOMI_GPIO          (17u)
#define BRD_GPIO_SPIA_SOMI               ((Uint32_t)GPIO_17_SPIA_SOMI)

#define BRD_GPIO_SPIA_CLK_GPIO           (18u)
#define BRD_GPIO_SPIA_CLK                ((Uint32_t)GPIO_18_SPIA_CLK)

#define BRD_GPIO_SPI_NAND_CS_GPIO        (19u)
#define BRD_GPIO_SPI_NAND_CS             ((Uint32_t)GPIO_19_GPIO19)

/*=== ENUMERATIONS ===========================================================*/

typedef enum GpioInstance_t
{
    GPIO_INSTANCE_SPIA_SIMO,
    GPIO_INSTANCE_SPIA_SOMI,
    GPIO_INSTANCE_SPIA_CLK,
    GPIO_INSTANCE_SPI_NAND_CS,
    GPIO_INSTANCE_COUNT
} GpioInstance_t;

/*=== EXTERNAL VARIABLE DEFINITIONS ==========================================*/

extern const GpioConfig_t BRD_GPIO_CONFIG[GPIO_INSTANCE_COUNT];

#endif /* BRD_GPIO_H */
