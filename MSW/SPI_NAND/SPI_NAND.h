/*
|===============================================================================
|
| File:         SPI_NAND.h
|
| Project:      SPI_NAND
|
| Processor:    TI TMS320F28003x
|
| Description:  Vendor-neutral SPI NAND command interface.
|
|=============================================================================*/

#ifndef SPI_NAND_H
#define SPI_NAND_H

/*=== INCLUDE FILES ==========================================================*/

#include "DATATYPE.h"
#include "DSPI.h"

/*=== #DEFINES ===============================================================*/

#define SPI_NAND_CMD_RESET              (0xFFu)
#define SPI_NAND_CMD_READ_ID            (0x9Fu)
#define SPI_NAND_CMD_GET_FEATURE        (0x0Fu)
#define SPI_NAND_CMD_SET_FEATURE        (0x1Fu)
#define SPI_NAND_CMD_WRITE_ENABLE       (0x06u)
#define SPI_NAND_CMD_PAGE_READ          (0x13u)
#define SPI_NAND_CMD_READ_CACHE         (0x03u)
#define SPI_NAND_CMD_PROGRAM_LOAD       (0x02u)
#define SPI_NAND_CMD_PROGRAM_EXECUTE    (0x10u)
#define SPI_NAND_CMD_BLOCK_ERASE        (0xD8u)

#define SPI_NAND_FEATURE_STATUS         (0xC0u)

/*=== ENUMERATIONS ===========================================================*/

typedef enum SpiNandStatus_t
{
    SPI_NAND_STATUS_OK,
    SPI_NAND_STATUS_NULL_PTR,
    SPI_NAND_STATUS_NOT_INITIALIZED,
    SPI_NAND_STATUS_SPI_ERROR,
    SPI_NAND_STATUS_TIMEOUT
} SpiNandStatus_t;

/*=== STRUCTURES =============================================================*/

typedef struct SpiNandConfig_t
{
    SpiHandle_t *spiHandle;
    Uint32_t     chipSelectGpio;
    Uint16_t     readIdDummyBytes;
    Uint16_t     readIdDummyValue;
    Uint16_t     busyMask;
    Uint32_t     readyPollLimit;
} SpiNandConfig_t;

typedef struct SpiNandHandle_t
{
    const SpiNandConfig_t *config;
    Uint16_t               statusRegister;
} SpiNandHandle_t;

/*=== EXTERNAL FUNCTION PROTOTYPES ===========================================*/

SpiNandStatus_t SPI_NAND_init(SpiNandHandle_t *handle,
                              const SpiNandConfig_t *config);
SpiNandStatus_t SPI_NAND_reset(SpiNandHandle_t *handle);
SpiNandStatus_t SPI_NAND_readId(SpiNandHandle_t *handle, Uint16_t *id,
                                Uint32_t length);
SpiNandStatus_t SPI_NAND_getFeature(SpiNandHandle_t *handle,
                                    Uint16_t featureAddress,
                                    Uint16_t *featureValue);
SpiNandStatus_t SPI_NAND_setFeature(SpiNandHandle_t *handle,
                                    Uint16_t featureAddress,
                                    Uint16_t featureValue);
SpiNandStatus_t SPI_NAND_writeEnable(SpiNandHandle_t *handle);
SpiNandStatus_t SPI_NAND_waitReady(SpiNandHandle_t *handle);
SpiNandStatus_t SPI_NAND_pageRead(SpiNandHandle_t *handle, Uint32_t rowAddress);
SpiNandStatus_t SPI_NAND_readCache(SpiNandHandle_t *handle,
                                   Uint16_t columnAddress,
                                   Uint16_t *data,
                                   Uint32_t length);
SpiNandStatus_t SPI_NAND_programLoad(SpiNandHandle_t *handle,
                                     Uint16_t columnAddress,
                                     const Uint16_t *data,
                                     Uint32_t length);
SpiNandStatus_t SPI_NAND_programExecute(SpiNandHandle_t *handle,
                                        Uint32_t rowAddress);
SpiNandStatus_t SPI_NAND_blockErase(SpiNandHandle_t *handle,
                                    Uint32_t rowAddress);

#endif /* SPI_NAND_H */
