#ifndef BRD_GPIO_H
#define BRD_GPIO_H

#include "DGPIO.h"

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

#define BRD_GPIO_W25N01GVSFIG_CS_GPIO    (19u)
#define BRD_GPIO_W25N01GVSFIG_CS         ((Uint32_t)GPIO_19_GPIO19)

#define BRD_GPIO_SCIA_RX_GPIO            (28u)
#define BRD_GPIO_SCIA_RX                 ((Uint32_t)GPIO_28_SCIA_RX)
#define BRD_GPIO_SCIA_TX_GPIO            (29u)
#define BRD_GPIO_SCIA_TX                 ((Uint32_t)GPIO_29_SCIA_TX)

typedef enum GpioInstance_t
{
    GPIO_INSTANCE_SPIA_SIMO,
    GPIO_INSTANCE_SPIA_SOMI,
    GPIO_INSTANCE_SPIA_CLK,
    GPIO_INSTANCE_W25N01GVSFIG_CS,
    GPIO_INSTANCE_SCIA_RX,
    GPIO_INSTANCE_SCIA_TX,
    GPIO_INSTANCE_COUNT
} GpioInstance_t;

extern const GpioConfig_t BRD_GPIO_CONFIG[GPIO_INSTANCE_COUNT];

#endif /* BRD_GPIO_H */
