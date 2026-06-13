/*
|===============================================================================
|
| File:         BRD_SPI.h
|
| Project:      SPI_NAND
|
| Processor:    TI TMS320F28003x
|
| Description:  SPI peripheral configuration.
|
|=============================================================================*/

#ifndef BRD_SPI_H
#define BRD_SPI_H

/*=== INCLUDE FILES ==========================================================*/

#include "DSPI.h"

/*=== EXTERNAL VARIABLE DEFINITIONS ==========================================*/

extern SpiHandle_t BRD_SPI_HANDLE;
extern const SpiConfig_t BRD_SPI_CONFIG;

#endif /* BRD_SPI_H */
