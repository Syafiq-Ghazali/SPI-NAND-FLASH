/*
|===============================================================================
|
| File:         BRD.h
|
| Project:      SPI_NAND
|
| Processor:    TI TMS320F28003x
|
| Description:  Top-level board configuration interface.
|
|=============================================================================*/

#ifndef BRD_H
#define BRD_H

/*=== INCLUDE FILES ==========================================================*/

#include "BRD_GPIO.h"
#include "BRD_MCU.h"
#include "BRD_SPI.h"
#include "BRD_W25N01GVSFIG.h"

/*=== EXTERNAL FUNCTION PROTOTYPES ===========================================*/

void BRD_init(void);

#endif /* BRD_H */
