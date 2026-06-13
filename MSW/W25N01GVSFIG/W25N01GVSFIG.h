/*
|===============================================================================
|
| File:         W25N01GVSFIG.h
|
| Project:      SPI_NAND
|
| Processor:    TI TMS320F28003x
|
| Description:  Winbond W25N01GVSFIG SPI NAND driver interface template.
|
|=============================================================================*/

#ifndef W25N01GVSFIG_H
#define W25N01GVSFIG_H

/*=== INCLUDE FILES ==========================================================*/

#include "DATATYPE.h"
#include "DSPI.h"

/*=== #DEFINES ===============================================================*/

#define W25N01GVSFIG_PAGE_DATA_SIZE       (2048u)
#define W25N01GVSFIG_PAGE_SPARE_SIZE      (64u)
#define W25N01GVSFIG_PAGE_TOTAL_SIZE      (2112u)
#define W25N01GVSFIG_SECTOR_DATA_SIZE     (512u)
#define W25N01GVSFIG_SECTORS_PER_PAGE     (4u)
#define W25N01GVSFIG_PAGES_PER_BLOCK      (64u)
#define W25N01GVSFIG_BLOCK_COUNT          (1024u)
#define W25N01GVSFIG_PAGE_COUNT           (65536uL)

#define W25N01GVSFIG_MANUFACTURER_ID      (0xEFu)
#define W25N01GVSFIG_DEVICE_ID_HIGH       (0xAAu)
#define W25N01GVSFIG_DEVICE_ID_LOW        (0x21u)

/*=== ENUMERATIONS ===========================================================*/

typedef enum W25N01GVSFIGStatus_t
{
    W25N01GVSFIG_STATUS_OK,
    W25N01GVSFIG_STATUS_ECC_CORRECTED,
    W25N01GVSFIG_STATUS_NULL_PTR,
    W25N01GVSFIG_STATUS_NOT_INITIALIZED,
    W25N01GVSFIG_STATUS_INVALID_CONFIG,
    W25N01GVSFIG_STATUS_INVALID_ADDRESS,
    W25N01GVSFIG_STATUS_INVALID_LENGTH,
    W25N01GVSFIG_STATUS_INVALID_BIT_TRANSITION,
    W25N01GVSFIG_STATUS_SPI_ERROR,
    W25N01GVSFIG_STATUS_TIMEOUT,
    W25N01GVSFIG_STATUS_ID_MISMATCH,
    W25N01GVSFIG_STATUS_WRITE_PROTECTED,
    W25N01GVSFIG_STATUS_BAD_BLOCK,
    W25N01GVSFIG_STATUS_PROGRAM_FAILED,
    W25N01GVSFIG_STATUS_ERASE_FAILED,
    W25N01GVSFIG_STATUS_ECC_UNCORRECTABLE
} W25N01GVSFIGStatus_t;

/*=== STRUCTURES =============================================================*/

typedef struct W25N01GVSFIGId_t
{
    Uint16_t manufacturer;
    Uint16_t deviceHigh;
    Uint16_t deviceLow;
} W25N01GVSFIGId_t;

typedef struct W25N01GVSFIGConfig_t
{
    SpiHandle_t *spiHandle;
    Uint32_t     chipSelectGpio;
    Uint32_t     readyPollLimit;
    Bool_t       unlockBlocksOnInit;
} W25N01GVSFIGConfig_t;

typedef struct W25N01GVSFIGHandle_t
{
    const W25N01GVSFIGConfig_t *config;
    Uint16_t                     statusRegister;
} W25N01GVSFIGHandle_t;

/*=== EXTERNAL FUNCTION PROTOTYPES ===========================================*/

W25N01GVSFIGStatus_t W25N01GVSFIG_init
(
    W25N01GVSFIGHandle_t *handle,
    const W25N01GVSFIGConfig_t *config
);

W25N01GVSFIGStatus_t W25N01GVSFIG_readId
(
    W25N01GVSFIGHandle_t *handle,
    W25N01GVSFIGId_t *id
);

W25N01GVSFIGStatus_t W25N01GVSFIG_readPage
(
    W25N01GVSFIGHandle_t *handle,
    Uint32_t page,
    Uint16_t offset,
    Uint16_t *data,
    Uint32_t length
);

W25N01GVSFIGStatus_t W25N01GVSFIG_programPage
(
    W25N01GVSFIGHandle_t *handle,
    Uint32_t page,
    Uint16_t offset,
    const Uint16_t *data,
    Uint32_t length
);

W25N01GVSFIGStatus_t W25N01GVSFIG_eraseBlock
(
    W25N01GVSFIGHandle_t *handle,
    Uint16_t block
);

W25N01GVSFIGStatus_t W25N01GVSFIG_isBadBlock
(
    W25N01GVSFIGHandle_t *handle,
    Uint16_t block,
    Bool_t *isBadBlock
);

#endif /* W25N01GVSFIG_H */
