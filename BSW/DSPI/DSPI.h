/*
|===============================================================================
|
| File:         DSPI.h
|
| Project:      SPI_NAND
|
| Processor:    TI TMS320F28003x
|
| Description:  Blocking SPI driver interface.
|
|=============================================================================*/

#ifndef DSPI_H
#define DSPI_H

/*=== INCLUDE FILES ==========================================================*/

#include "DATATYPE.h"
#include "DGPIO.h"
#include "driverlib.h"

/*=== #DEFINES ===============================================================*/

#define DSPI_DUMMY_DATA                 (0xFFu)

/*=== ENUMERATIONS ===========================================================*/

typedef enum SpiStatus_t
{
    SPI_STATUS_OK,
    SPI_STATUS_NULL_PTR,
    SPI_STATUS_NOT_INITIALIZED,
    SPI_STATUS_INVALID_CONFIG
} SpiStatus_t;

/*=== STRUCTURES =============================================================*/

typedef struct SpiConfig_t
{
    Uint32_t             base;
    Uint32_t             sclkFrequency;
    Uint32_t             bitRate;
    Uint16_t             dataWidth;
    SPI_TransferProtocol transferProtocol;
    SPI_Mode             mode;
    const GpioConfig_t  *simoGpioConfig;
    const GpioConfig_t  *somiGpioConfig;
    const GpioConfig_t  *clkGpioConfig;
} SpiConfig_t;

typedef struct SpiHandle_t
{
    const SpiConfig_t *config;
} SpiHandle_t;

/*=== EXTERNAL FUNCTION PROTOTYPES ===========================================*/

SpiStatus_t DSPI_init(SpiHandle_t *handle, const SpiConfig_t *config);
SpiStatus_t DSPI_select(SpiHandle_t *handle, Uint32_t chipSelectGpio);
SpiStatus_t DSPI_deselect(SpiHandle_t *handle, Uint32_t chipSelectGpio);
SpiStatus_t DSPI_exchange(SpiHandle_t *handle, const Uint16_t *txData,
                          Uint16_t *rxData, Uint32_t length);
SpiStatus_t DSPI_transfer(SpiHandle_t *handle, const Uint16_t *txData,
                          Uint16_t *rxData, Uint32_t length,
                          Uint32_t chipSelectGpio);

#endif /* DSPI_H */
