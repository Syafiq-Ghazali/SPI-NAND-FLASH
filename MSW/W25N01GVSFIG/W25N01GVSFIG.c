/*
|===============================================================================
|
| File:         W25N01GVSFIG.c
|
| Project:      SPI_NAND
|
| Processor:    TI TMS320F28003x
|
| Description:  Winbond W25N01GVSFIG SPI NAND driver template.
|
|=============================================================================*/

/*=== INCLUDE FILES ==========================================================*/

#include "W25N01GVSFIG.h"
#include "DMCU.h"

/*=== #DEFINES ===============================================================*/

#define W25N01GVSFIG_CMD_RESET               (0xFFu)
#define W25N01GVSFIG_CMD_READ_ID             (0x9Fu)
#define W25N01GVSFIG_CMD_GET_FEATURE         (0x0Fu)
#define W25N01GVSFIG_CMD_SET_FEATURE         (0x1Fu)
#define W25N01GVSFIG_CMD_WRITE_ENABLE        (0x06u)
#define W25N01GVSFIG_CMD_PAGE_DATA_READ      (0x13u)
#define W25N01GVSFIG_CMD_READ_DATA           (0x03u)
#define W25N01GVSFIG_CMD_PROGRAM_DATA_LOAD   (0x02u)
#define W25N01GVSFIG_CMD_PROGRAM_EXECUTE     (0x10u)
#define W25N01GVSFIG_CMD_BLOCK_ERASE         (0xD8u)

#define W25N01GVSFIG_FEATURE_PROTECTION      (0xA0u)
#define W25N01GVSFIG_FEATURE_CONFIGURATION   (0xB0u)
#define W25N01GVSFIG_FEATURE_STATUS          (0xC0u)

#define W25N01GVSFIG_PROTECTION_BLOCK_MASK   (0x7Cu)

#define W25N01GVSFIG_CONFIGURATION_OTP_E     (0x40u)
#define W25N01GVSFIG_CONFIGURATION_ECC_E     (0x10u)
#define W25N01GVSFIG_CONFIGURATION_BUF       (0x08u)

#define W25N01GVSFIG_STATUS_ECC_MASK         (0x30u)
#define W25N01GVSFIG_STATUS_ECC_CORRECTED_BITS \
                                                (0x10u)
#define W25N01GVSFIG_STATUS_P_FAIL           (0x08u)
#define W25N01GVSFIG_STATUS_E_FAIL           (0x04u)
#define W25N01GVSFIG_STATUS_WEL              (0x02u)
#define W25N01GVSFIG_STATUS_BUSY             (0x01u)

#define W25N01GVSFIG_BAD_BLOCK_MARKER_COLUMN \
                                                (W25N01GVSFIG_PAGE_DATA_SIZE)
#define W25N01GVSFIG_BAD_BLOCK_MARKER_SIZE      (2u)

#define W25N01GVSFIG_POWER_UP_DELAY_US          (1000u)
#define W25N01GVSFIG_RESET_DELAY_US             (500u)
#define W25N01GVSFIG_VERIFY_BUFFER_SIZE          (16u)

/*=== PRIVATE FUNCTION PROTOTYPES ============================================*/

static W25N01GVSFIGStatus_t F_validateHandle
(
    const W25N01GVSFIGHandle_t *handle
);

static W25N01GVSFIGStatus_t F_command
(
    W25N01GVSFIGHandle_t *handle,
    Uint16_t command
);

static W25N01GVSFIGStatus_t F_rowCommand
(
    W25N01GVSFIGHandle_t *handle,
    Uint16_t command,
    Uint32_t page
);

static W25N01GVSFIGStatus_t F_getFeature
(
    W25N01GVSFIGHandle_t *handle,
    Uint16_t address,
    Uint16_t *value
);

static W25N01GVSFIGStatus_t F_setFeature
(
    W25N01GVSFIGHandle_t *handle,
    Uint16_t address,
    Uint16_t value
);

static W25N01GVSFIGStatus_t F_writeEnable
(
    W25N01GVSFIGHandle_t *handle
);

static W25N01GVSFIGStatus_t F_waitReady
(
    W25N01GVSFIGHandle_t *handle,
    Uint16_t *statusRegister
);

static W25N01GVSFIGStatus_t F_pageDataRead
(
    W25N01GVSFIGHandle_t *handle,
    Uint32_t page,
    Uint16_t *statusRegister
);

static W25N01GVSFIGStatus_t F_readData
(
    W25N01GVSFIGHandle_t *handle,
    Uint16_t column,
    Uint16_t *data,
    Uint32_t length
);

static W25N01GVSFIGStatus_t F_programDataLoad
(
    W25N01GVSFIGHandle_t *handle,
    Uint16_t column,
    const Uint16_t *data,
    Uint32_t length
);

static W25N01GVSFIGStatus_t F_validateProgramData
(
    W25N01GVSFIGHandle_t *handle,
    Uint32_t page,
    Uint16_t column,
    const Uint16_t *data,
    Uint32_t length
);

static W25N01GVSFIGStatus_t F_isWriteProtected
(
    W25N01GVSFIGHandle_t *handle,
    Bool_t *isWriteProtected
);

static W25N01GVSFIGStatus_t F_fromSpiStatus
(
    SpiStatus_t status
);

/*=== FUNCTION DEFINITIONS ===================================================*/

/*
|===============================================================================
|
| Function:         W25N01GVSFIG_init
|
| Description:      Initialize and validate the W25N01GVSFIG device. Enables
|                   internal ECC and buffer-read mode, and optionally removes
|                   volatile block protection.
|
| Dependencies:     The SPI and chip-select GPIO drivers must be initialized.
|
| Notes:            The handle is invalidated if initialization fails.
|
| Side Effects:     Resets the device and updates its configuration and
|                   protection feature registers.
|
| Return Value:     W25N01GVSFIGStatus_t indicating the initialization result.
|
|===============================================================================
| Variable        Access (I/O)    Description
|-------------------------------------------------------------------------------
| handle          I/O             Device instance handle
| config          I               Device instance configuration
|=============================================================================*/

W25N01GVSFIGStatus_t W25N01GVSFIG_init
(
    W25N01GVSFIGHandle_t *handle,
    const W25N01GVSFIGConfig_t *config
)
{
    W25N01GVSFIGId_t id;
    W25N01GVSFIGStatus_t status;
    Uint16_t feature;

    if ((handle == NULL) || (config == NULL) || (config->spiHandle == NULL))
    {
        return W25N01GVSFIG_STATUS_NULL_PTR;
    }

    if ((config->spiHandle->config == NULL) ||
        (config->readyPollLimit == 0u))
    {
        return W25N01GVSFIG_STATUS_INVALID_CONFIG;
    }

    handle->config = config;
    handle->statusRegister = 0u;

    DMCU_DELAY_US(W25N01GVSFIG_POWER_UP_DELAY_US);

    status = F_waitReady(handle, NULL);
    if (status == W25N01GVSFIG_STATUS_OK)
    {
        status = F_command(handle, W25N01GVSFIG_CMD_RESET);
    }
    if (status == W25N01GVSFIG_STATUS_OK)
    {
        DMCU_DELAY_US(W25N01GVSFIG_RESET_DELAY_US);
        status = F_waitReady(handle, NULL);
    }
    if (status == W25N01GVSFIG_STATUS_OK)
    {
        status = W25N01GVSFIG_readId(handle, &id);
    }
    if ((status == W25N01GVSFIG_STATUS_OK) &&
        ((id.manufacturer != W25N01GVSFIG_MANUFACTURER_ID) ||
         (id.deviceHigh != W25N01GVSFIG_DEVICE_ID_HIGH) ||
         (id.deviceLow != W25N01GVSFIG_DEVICE_ID_LOW)))
    {
        status = W25N01GVSFIG_STATUS_ID_MISMATCH;
    }

    if (status == W25N01GVSFIG_STATUS_OK)
    {
        status = F_getFeature(handle,
                              W25N01GVSFIG_FEATURE_CONFIGURATION,
                              &feature);
    }
    if (status == W25N01GVSFIG_STATUS_OK)
    {
        feature &= (Uint16_t)(~W25N01GVSFIG_CONFIGURATION_OTP_E);
        feature |= (W25N01GVSFIG_CONFIGURATION_ECC_E |
                    W25N01GVSFIG_CONFIGURATION_BUF);
        status = F_setFeature(handle,
                              W25N01GVSFIG_FEATURE_CONFIGURATION,
                              feature);
    }
    if (status == W25N01GVSFIG_STATUS_OK)
    {
        status = F_getFeature(handle,
                              W25N01GVSFIG_FEATURE_CONFIGURATION,
                              &feature);
    }
    if ((status == W25N01GVSFIG_STATUS_OK) &&
        (((feature & W25N01GVSFIG_CONFIGURATION_ECC_E) == 0u) ||
         ((feature & W25N01GVSFIG_CONFIGURATION_BUF) == 0u) ||
         ((feature & W25N01GVSFIG_CONFIGURATION_OTP_E) != 0u)))
    {
        status = W25N01GVSFIG_STATUS_INVALID_CONFIG;
    }

    if ((status == W25N01GVSFIG_STATUS_OK) &&
        (config->unlockBlocksOnInit == TRUE))
    {
        status = F_getFeature(handle,
                              W25N01GVSFIG_FEATURE_PROTECTION,
                              &feature);
        if (status == W25N01GVSFIG_STATUS_OK)
        {
            feature &=
                (Uint16_t)(~W25N01GVSFIG_PROTECTION_BLOCK_MASK);
            status = F_setFeature(handle,
                                  W25N01GVSFIG_FEATURE_PROTECTION,
                                  feature);
        }
        if (status == W25N01GVSFIG_STATUS_OK)
        {
            status = F_getFeature(handle,
                                  W25N01GVSFIG_FEATURE_PROTECTION,
                                  &feature);
        }
        if ((status == W25N01GVSFIG_STATUS_OK) &&
            ((feature & W25N01GVSFIG_PROTECTION_BLOCK_MASK) != 0u))
        {
            status = W25N01GVSFIG_STATUS_WRITE_PROTECTED;
        }
    }

    if (status != W25N01GVSFIG_STATUS_OK)
    {
        handle->config = NULL;
    }

    return status;
}

/*
|===============================================================================
|
| Function:         W25N01GVSFIG_readId
|
| Description:      Read the manufacturer and device identification bytes.
|
| Dependencies:     W25N01GVSFIG_init or a valid initialized handle.
|
| Notes:
|
| Side Effects:     Performs one SPI transaction.
|
| Return Value:     W25N01GVSFIGStatus_t indicating the read result.
|
|===============================================================================
| Variable        Access (I/O)    Description
|-------------------------------------------------------------------------------
| handle          I               Device instance handle
| id              O               Manufacturer and device identification
|=============================================================================*/

W25N01GVSFIGStatus_t W25N01GVSFIG_readId
(
    W25N01GVSFIGHandle_t *handle,
    W25N01GVSFIGId_t *id
)
{
    const Uint16_t command[] =
    {
        W25N01GVSFIG_CMD_READ_ID,
        0u
    };
    Uint16_t idData[3u];
    W25N01GVSFIGStatus_t status;
    SpiStatus_t spiStatus;

    status = F_validateHandle(handle);
    if (status != W25N01GVSFIG_STATUS_OK)
    {
        return status;
    }
    if (id == NULL)
    {
        return W25N01GVSFIG_STATUS_NULL_PTR;
    }

    spiStatus = DSPI_select(handle->config->spiHandle,
                            handle->config->chipSelectGpio);
    if (spiStatus == SPI_STATUS_OK)
    {
        spiStatus = DSPI_exchange(handle->config->spiHandle,
                                  command,
                                  NULL,
                                  2u);
    }
    if (spiStatus == SPI_STATUS_OK)
    {
        spiStatus = DSPI_exchange(handle->config->spiHandle,
                                  NULL,
                                  idData,
                                  3u);
    }
    (void)DSPI_deselect(handle->config->spiHandle,
                        handle->config->chipSelectGpio);

    status = F_fromSpiStatus(spiStatus);
    if (status == W25N01GVSFIG_STATUS_OK)
    {
        id->manufacturer = idData[0u];
        id->deviceHigh = idData[1u];
        id->deviceLow = idData[2u];
    }

    return status;
}

/*
|===============================================================================
|
| Function:         W25N01GVSFIG_readPage
|
| Description:      Transfer a NAND page into the internal data buffer, check
|                   its ECC result, and read the requested data bytes.
|
| Dependencies:     A successfully initialized device.
|
| Notes:            Reads are limited to the 2,048-byte page data area.
|
| Side Effects:     Updates the cached device status in the handle.
|
| Return Value:     W25N01GVSFIG_STATUS_ECC_CORRECTED when valid data required
|                   correction, otherwise the applicable operation status.
|
|===============================================================================
| Variable        Access (I/O)    Description
|-------------------------------------------------------------------------------
| handle          I/O             Device instance handle
| page            I               Page number from 0 to 65,535
| offset          I               Starting byte offset within the page
| data            O               Destination data buffer
| length          I               Number of bytes to read
|=============================================================================*/

W25N01GVSFIGStatus_t W25N01GVSFIG_readPage
(
    W25N01GVSFIGHandle_t *handle,
    Uint32_t page,
    Uint16_t offset,
    Uint16_t *data,
    Uint32_t length
)
{
    W25N01GVSFIGStatus_t status;
    Uint16_t statusRegister;
    Uint16_t eccStatus;

    status = F_validateHandle(handle);
    if (status != W25N01GVSFIG_STATUS_OK)
    {
        return status;
    }
    if (data == NULL)
    {
        return W25N01GVSFIG_STATUS_NULL_PTR;
    }
    if (page >= W25N01GVSFIG_PAGE_COUNT)
    {
        return W25N01GVSFIG_STATUS_INVALID_ADDRESS;
    }
    if ((length == 0u) ||
        (((Uint32_t)offset + length) > W25N01GVSFIG_PAGE_DATA_SIZE))
    {
        return W25N01GVSFIG_STATUS_INVALID_LENGTH;
    }
    status = F_pageDataRead(handle, page, &statusRegister);
    if (status != W25N01GVSFIG_STATUS_OK)
    {
        return status;
    }

    eccStatus = statusRegister & W25N01GVSFIG_STATUS_ECC_MASK;
    if ((eccStatus != 0u) &&
        (eccStatus != W25N01GVSFIG_STATUS_ECC_CORRECTED_BITS))
    {
        return W25N01GVSFIG_STATUS_ECC_UNCORRECTABLE;
    }

    status = F_readData(handle, offset, data, length);
    if ((status == W25N01GVSFIG_STATUS_OK) &&
        (eccStatus == W25N01GVSFIG_STATUS_ECC_CORRECTED_BITS))
    {
        status = W25N01GVSFIG_STATUS_ECC_CORRECTED;
    }

    return status;
}

/*
|===============================================================================
|
| Function:         W25N01GVSFIG_programPage
|
| Description:      Validate and program one or more 512-byte sectors within
|                   a page using the device data buffer.
|
| Dependencies:     A successfully initialized and writable device.
|
| Notes:            The page offset and length must be 512-byte aligned.
|                   Existing zero bits cannot be programmed back to one.
|
| Side Effects:     Programs NAND data and updates the cached device status.
|
| Return Value:     W25N01GVSFIGStatus_t indicating the program result.
|
|===============================================================================
| Variable        Access (I/O)    Description
|-------------------------------------------------------------------------------
| handle          I/O             Device instance handle
| page            I               Page number from 0 to 65,535
| offset          I               Starting byte offset within the page
| data            I               Source data buffer
| length          I               Number of bytes to program
|=============================================================================*/

W25N01GVSFIGStatus_t W25N01GVSFIG_programPage
(
    W25N01GVSFIGHandle_t *handle,
    Uint32_t page,
    Uint16_t offset,
    const Uint16_t *data,
    Uint32_t length
)
{
    W25N01GVSFIGStatus_t status;
    Bool_t isBadBlock;
    Bool_t isWriteProtected;
    Uint16_t statusRegister;

    status = F_validateHandle(handle);
    if (status != W25N01GVSFIG_STATUS_OK)
    {
        return status;
    }
    if (data == NULL)
    {
        return W25N01GVSFIG_STATUS_NULL_PTR;
    }
    if (page >= W25N01GVSFIG_PAGE_COUNT)
    {
        return W25N01GVSFIG_STATUS_INVALID_ADDRESS;
    }
    if ((length == 0u) ||
        (((Uint32_t)offset + length) > W25N01GVSFIG_PAGE_DATA_SIZE))
    {
        return W25N01GVSFIG_STATUS_INVALID_LENGTH;
    }
    if (((offset % W25N01GVSFIG_SECTOR_DATA_SIZE) != 0u) ||
        ((length % W25N01GVSFIG_SECTOR_DATA_SIZE) != 0u))
    {
        return W25N01GVSFIG_STATUS_INVALID_LENGTH;
    }

    status = W25N01GVSFIG_isBadBlock(
        handle,
        (Uint16_t)(page / W25N01GVSFIG_PAGES_PER_BLOCK),
        &isBadBlock);
    if (status != W25N01GVSFIG_STATUS_OK)
    {
        return status;
    }
    if (isBadBlock == TRUE)
    {
        return W25N01GVSFIG_STATUS_BAD_BLOCK;
    }

    status = F_isWriteProtected(handle, &isWriteProtected);
    if (status != W25N01GVSFIG_STATUS_OK)
    {
        return status;
    }
    if (isWriteProtected == TRUE)
    {
        return W25N01GVSFIG_STATUS_WRITE_PROTECTED;
    }

    status = F_validateProgramData(handle,
                                   page,
                                   offset,
                                   data,
                                   length);
    if (status == W25N01GVSFIG_STATUS_OK)
    {
        status = F_writeEnable(handle);
    }
    if (status == W25N01GVSFIG_STATUS_OK)
    {
        status = F_programDataLoad(handle, offset, data, length);
    }
    if (status == W25N01GVSFIG_STATUS_OK)
    {
        status = F_rowCommand(handle,
                              W25N01GVSFIG_CMD_PROGRAM_EXECUTE,
                              page);
    }
    if (status == W25N01GVSFIG_STATUS_OK)
    {
        status = F_waitReady(handle, &statusRegister);
    }
    if ((status == W25N01GVSFIG_STATUS_OK) &&
        ((statusRegister & W25N01GVSFIG_STATUS_P_FAIL) != 0u))
    {
        status = W25N01GVSFIG_STATUS_PROGRAM_FAILED;
    }

    return status;
}

/*
|===============================================================================
|
| Function:         W25N01GVSFIG_eraseBlock
|
| Description:      Erase one complete 128 KiB NAND block after confirming
|                   that it is writable and not marked bad.
|
| Dependencies:     A successfully initialized and writable device.
|
| Notes:            Erasing restores every bit in the block to one.
|
| Side Effects:     Erases 64 pages and updates the cached device status.
|
| Return Value:     W25N01GVSFIGStatus_t indicating the erase result.
|
|===============================================================================
| Variable        Access (I/O)    Description
|-------------------------------------------------------------------------------
| handle          I/O             Device instance handle
| block           I               Block number from 0 to 1,023
|=============================================================================*/

W25N01GVSFIGStatus_t W25N01GVSFIG_eraseBlock
(
    W25N01GVSFIGHandle_t *handle,
    Uint16_t block
)
{
    W25N01GVSFIGStatus_t status;
    Bool_t isBadBlock;
    Bool_t isWriteProtected;
    Uint16_t statusRegister;
    Uint32_t firstPage;

    status = F_validateHandle(handle);
    if (status != W25N01GVSFIG_STATUS_OK)
    {
        return status;
    }
    if (block >= W25N01GVSFIG_BLOCK_COUNT)
    {
        return W25N01GVSFIG_STATUS_INVALID_ADDRESS;
    }

    status = W25N01GVSFIG_isBadBlock(handle, block, &isBadBlock);
    if (status != W25N01GVSFIG_STATUS_OK)
    {
        return status;
    }
    if (isBadBlock == TRUE)
    {
        return W25N01GVSFIG_STATUS_BAD_BLOCK;
    }

    status = F_isWriteProtected(handle, &isWriteProtected);
    if (status != W25N01GVSFIG_STATUS_OK)
    {
        return status;
    }
    if (isWriteProtected == TRUE)
    {
        return W25N01GVSFIG_STATUS_WRITE_PROTECTED;
    }

    firstPage = (Uint32_t)block * W25N01GVSFIG_PAGES_PER_BLOCK;

    status = F_writeEnable(handle);
    if (status == W25N01GVSFIG_STATUS_OK)
    {
        status = F_rowCommand(handle,
                              W25N01GVSFIG_CMD_BLOCK_ERASE,
                              firstPage);
    }
    if (status == W25N01GVSFIG_STATUS_OK)
    {
        status = F_waitReady(handle, &statusRegister);
    }
    if ((status == W25N01GVSFIG_STATUS_OK) &&
        ((statusRegister & W25N01GVSFIG_STATUS_E_FAIL) != 0u))
    {
        status = W25N01GVSFIG_STATUS_ERASE_FAILED;
    }

    return status;
}

/*
|===============================================================================
|
| Function:         W25N01GVSFIG_isBadBlock
|
| Description:      Check the first page spare-area marker and ECC result to
|                   determine whether a block must not be used.
|
| Dependencies:     A successfully initialized device.
|
| Notes:            A non-0xFF marker or uncorrectable ECC marks the block bad.
|
| Side Effects:     Loads the block's first page and updates cached status.
|
| Return Value:     W25N01GVSFIGStatus_t indicating whether the check ran
|                   successfully. The block result is returned separately.
|
|===============================================================================
| Variable        Access (I/O)    Description
|-------------------------------------------------------------------------------
| handle          I/O             Device instance handle
| block           I               Block number from 0 to 1,023
| isBadBlock      O               TRUE when the block must not be used
|=============================================================================*/

W25N01GVSFIGStatus_t W25N01GVSFIG_isBadBlock
(
    W25N01GVSFIGHandle_t *handle,
    Uint16_t block,
    Bool_t *isBadBlock
)
{
    W25N01GVSFIGStatus_t status;
    Uint16_t marker[W25N01GVSFIG_BAD_BLOCK_MARKER_SIZE];
    Uint16_t statusRegister;
    Uint16_t eccStatus;
    Uint32_t firstPage;

    status = F_validateHandle(handle);
    if (status != W25N01GVSFIG_STATUS_OK)
    {
        return status;
    }
    if (isBadBlock == NULL)
    {
        return W25N01GVSFIG_STATUS_NULL_PTR;
    }
    if (block >= W25N01GVSFIG_BLOCK_COUNT)
    {
        return W25N01GVSFIG_STATUS_INVALID_ADDRESS;
    }

    *isBadBlock = FALSE;
    firstPage = (Uint32_t)block * W25N01GVSFIG_PAGES_PER_BLOCK;

    status = F_pageDataRead(handle, firstPage, &statusRegister);
    if (status != W25N01GVSFIG_STATUS_OK)
    {
        return status;
    }

    eccStatus = statusRegister & W25N01GVSFIG_STATUS_ECC_MASK;
    if ((eccStatus != 0u) &&
        (eccStatus != W25N01GVSFIG_STATUS_ECC_CORRECTED_BITS))
    {
        *isBadBlock = TRUE;
        return W25N01GVSFIG_STATUS_OK;
    }

    status = F_readData(handle,
                        W25N01GVSFIG_BAD_BLOCK_MARKER_COLUMN,
                        marker,
                        W25N01GVSFIG_BAD_BLOCK_MARKER_SIZE);
    if (status == W25N01GVSFIG_STATUS_OK)
    {
        *isBadBlock = ((marker[0u] != 0xFFu) ||
                       (marker[1u] != 0xFFu));
    }

    return status;
}

/*
|===============================================================================
|
| Function:         F_validateHandle
|
| Description:      Validate the device handle and its SPI dependency.
|
| Dependencies:
|
| Notes:
|
| Side Effects:     None.
|
| Return Value:     W25N01GVSFIG_STATUS_OK when the handle is usable.
|
|===============================================================================
| Variable        Access (I/O)    Description
|-------------------------------------------------------------------------------
| handle          I               Device instance handle
|=============================================================================*/

static W25N01GVSFIGStatus_t F_validateHandle
(
    const W25N01GVSFIGHandle_t *handle
)
{
    if (handle == NULL)
    {
        return W25N01GVSFIG_STATUS_NULL_PTR;
    }
    if ((handle->config == NULL) ||
        (handle->config->spiHandle == NULL) ||
        (handle->config->spiHandle->config == NULL))
    {
        return W25N01GVSFIG_STATUS_NOT_INITIALIZED;
    }

    return W25N01GVSFIG_STATUS_OK;
}

/*
|===============================================================================
|
| Function:         F_command
|
| Description:      Send a one-byte device command in a complete SPI
|                   transaction.
|
| Dependencies:     A valid initialized device handle.
|
| Notes:
|
| Side Effects:     Toggles chip select and transfers one SPI byte.
|
| Return Value:     W25N01GVSFIGStatus_t converted from the SPI result.
|
|===============================================================================
| Variable        Access (I/O)    Description
|-------------------------------------------------------------------------------
| handle          I               Device instance handle
| command         I               Command opcode
|=============================================================================*/

static W25N01GVSFIGStatus_t F_command
(
    W25N01GVSFIGHandle_t *handle,
    Uint16_t command
)
{
    W25N01GVSFIGStatus_t status;

    status = F_validateHandle(handle);
    if (status != W25N01GVSFIG_STATUS_OK)
    {
        return status;
    }

    return F_fromSpiStatus(
        DSPI_transfer(handle->config->spiHandle,
                      &command,
                      NULL,
                      1u,
                      handle->config->chipSelectGpio));
}

/*
|===============================================================================
|
| Function:         F_rowCommand
|
| Description:      Send a command containing the device's 16-bit page
|                   address format and required leading dummy byte.
|
| Dependencies:     A valid initialized device handle.
|
| Notes:
|
| Side Effects:     Performs one SPI transaction.
|
| Return Value:     W25N01GVSFIGStatus_t converted from the SPI result.
|
|===============================================================================
| Variable        Access (I/O)    Description
|-------------------------------------------------------------------------------
| handle          I               Device instance handle
| command         I               Command opcode
| page            I               Page address
|=============================================================================*/

static W25N01GVSFIGStatus_t F_rowCommand
(
    W25N01GVSFIGHandle_t *handle,
    Uint16_t command,
    Uint32_t page
)
{
    const Uint16_t commandData[] =
    {
        command,
        0u,
        (Uint16_t)((page >> 8u) & 0xFFu),
        (Uint16_t)(page & 0xFFu)
    };

    return F_fromSpiStatus(
        DSPI_transfer(handle->config->spiHandle,
                      commandData,
                      NULL,
                      4u,
                      handle->config->chipSelectGpio));
}

/*
|===============================================================================
|
| Function:         F_getFeature
|
| Description:      Read one protection, configuration, or status feature
|                   register from the device.
|
| Dependencies:     A valid initialized device handle.
|
| Notes:
|
| Side Effects:     Performs one SPI transaction.
|
| Return Value:     W25N01GVSFIGStatus_t indicating the register read result.
|
|===============================================================================
| Variable        Access (I/O)    Description
|-------------------------------------------------------------------------------
| handle          I               Device instance handle
| address         I               Feature register address
| value           O               Feature register value
|=============================================================================*/

static W25N01GVSFIGStatus_t F_getFeature
(
    W25N01GVSFIGHandle_t *handle,
    Uint16_t address,
    Uint16_t *value
)
{
    const Uint16_t command[] =
    {
        W25N01GVSFIG_CMD_GET_FEATURE,
        address
    };
    W25N01GVSFIGStatus_t status;
    SpiStatus_t spiStatus;

    status = F_validateHandle(handle);
    if (status != W25N01GVSFIG_STATUS_OK)
    {
        return status;
    }
    if (value == NULL)
    {
        return W25N01GVSFIG_STATUS_NULL_PTR;
    }

    spiStatus = DSPI_select(handle->config->spiHandle,
                            handle->config->chipSelectGpio);
    if (spiStatus == SPI_STATUS_OK)
    {
        spiStatus = DSPI_exchange(handle->config->spiHandle,
                                  command,
                                  NULL,
                                  2u);
    }
    if (spiStatus == SPI_STATUS_OK)
    {
        spiStatus = DSPI_exchange(handle->config->spiHandle,
                                  NULL,
                                  value,
                                  1u);
    }
    (void)DSPI_deselect(handle->config->spiHandle,
                        handle->config->chipSelectGpio);

    return F_fromSpiStatus(spiStatus);
}

/*
|===============================================================================
|
| Function:         F_setFeature
|
| Description:      Write one writable device feature register and wait for
|                   the operation to complete.
|
| Dependencies:     A valid initialized device handle.
|
| Notes:
|
| Side Effects:     Sets WEL and modifies a device feature register.
|
| Return Value:     W25N01GVSFIGStatus_t indicating the register write result.
|
|===============================================================================
| Variable        Access (I/O)    Description
|-------------------------------------------------------------------------------
| handle          I/O             Device instance handle
| address         I               Feature register address
| value           I               New feature register value
|=============================================================================*/

static W25N01GVSFIGStatus_t F_setFeature
(
    W25N01GVSFIGHandle_t *handle,
    Uint16_t address,
    Uint16_t value
)
{
    const Uint16_t command[] =
    {
        W25N01GVSFIG_CMD_SET_FEATURE,
        address,
        value
    };
    W25N01GVSFIGStatus_t status;

    status = F_writeEnable(handle);
    if (status != W25N01GVSFIG_STATUS_OK)
    {
        return status;
    }

    status = F_fromSpiStatus(
        DSPI_transfer(handle->config->spiHandle,
                      command,
                      NULL,
                      3u,
                      handle->config->chipSelectGpio));
    if (status == W25N01GVSFIG_STATUS_OK)
    {
        status = F_waitReady(handle, NULL);
    }

    return status;
}

/*
|===============================================================================
|
| Function:         F_writeEnable
|
| Description:      Set and verify the device write-enable latch.
|
| Dependencies:     A valid initialized device handle.
|
| Notes:
|
| Side Effects:     Sets the volatile WEL status bit.
|
| Return Value:     W25N01GVSFIG_STATUS_WRITE_PROTECTED if WEL does not set.
|
|===============================================================================
| Variable        Access (I/O)    Description
|-------------------------------------------------------------------------------
| handle          I/O             Device instance handle
|=============================================================================*/

static W25N01GVSFIGStatus_t F_writeEnable
(
    W25N01GVSFIGHandle_t *handle
)
{
    W25N01GVSFIGStatus_t status;
    Uint16_t statusRegister;

    status = F_command(handle, W25N01GVSFIG_CMD_WRITE_ENABLE);
    if (status == W25N01GVSFIG_STATUS_OK)
    {
        status = F_getFeature(handle,
                              W25N01GVSFIG_FEATURE_STATUS,
                              &statusRegister);
    }
    if ((status == W25N01GVSFIG_STATUS_OK) &&
        ((statusRegister & W25N01GVSFIG_STATUS_WEL) == 0u))
    {
        status = W25N01GVSFIG_STATUS_WRITE_PROTECTED;
    }

    return status;
}

/*
|===============================================================================
|
| Function:         F_waitReady
|
| Description:      Poll the status feature register until the BUSY bit clears
|                   or the configured poll limit is reached.
|
| Dependencies:     A valid initialized device handle.
|
| Notes:            statusRegister may be NULL when the final value is unused.
|
| Side Effects:     Updates the cached status register in the handle.
|
| Return Value:     W25N01GVSFIG_STATUS_TIMEOUT if BUSY does not clear.
|
|===============================================================================
| Variable        Access (I/O)    Description
|-------------------------------------------------------------------------------
| handle          I/O             Device instance handle
| statusRegister  O               Optional final status register value
|=============================================================================*/

static W25N01GVSFIGStatus_t F_waitReady
(
    W25N01GVSFIGHandle_t *handle,
    Uint16_t *statusRegister
)
{
    W25N01GVSFIGStatus_t status;
    Uint16_t deviceStatus;
    Uint32_t poll;

    status = F_validateHandle(handle);
    if (status != W25N01GVSFIG_STATUS_OK)
    {
        return status;
    }

    for (poll = 0u; poll < handle->config->readyPollLimit; poll++)
    {
        status = F_getFeature(handle,
                              W25N01GVSFIG_FEATURE_STATUS,
                              &deviceStatus);
        if (status != W25N01GVSFIG_STATUS_OK)
        {
            return status;
        }

        handle->statusRegister = deviceStatus;
        if ((deviceStatus & W25N01GVSFIG_STATUS_BUSY) == 0u)
        {
            if (statusRegister != NULL)
            {
                *statusRegister = deviceStatus;
            }
            return W25N01GVSFIG_STATUS_OK;
        }
    }

    return W25N01GVSFIG_STATUS_TIMEOUT;
}

/*
|===============================================================================
|
| Function:         F_pageDataRead
|
| Description:      Transfer one NAND page into the internal data buffer and
|                   wait for the transfer to complete.
|
| Dependencies:     A valid initialized device handle.
|
| Notes:
|
| Side Effects:     Replaces the contents of the device data buffer.
|
| Return Value:     W25N01GVSFIGStatus_t indicating the transfer result.
|
|===============================================================================
| Variable        Access (I/O)    Description
|-------------------------------------------------------------------------------
| handle          I/O             Device instance handle
| page            I               Source NAND page
| statusRegister  O               Optional final status register value
|=============================================================================*/

static W25N01GVSFIGStatus_t F_pageDataRead
(
    W25N01GVSFIGHandle_t *handle,
    Uint32_t page,
    Uint16_t *statusRegister
)
{
    W25N01GVSFIGStatus_t status;

    status = F_rowCommand(handle,
                          W25N01GVSFIG_CMD_PAGE_DATA_READ,
                          page);
    if (status == W25N01GVSFIG_STATUS_OK)
    {
        status = F_waitReady(handle, statusRegister);
    }

    return status;
}

/*
|===============================================================================
|
| Function:         F_readData
|
| Description:      Read sequential bytes from the device's internal data
|                   buffer beginning at a column address.
|
| Dependencies:     The desired page must already be loaded into the buffer.
|
| Notes:            This helper can access the main and spare page areas.
|
| Side Effects:     Performs one SPI transaction.
|
| Return Value:     W25N01GVSFIGStatus_t indicating the buffer read result.
|
|===============================================================================
| Variable        Access (I/O)    Description
|-------------------------------------------------------------------------------
| handle          I               Device instance handle
| column          I               Starting data-buffer column
| data            O               Destination data buffer
| length          I               Number of bytes to read
|=============================================================================*/

static W25N01GVSFIGStatus_t F_readData
(
    W25N01GVSFIGHandle_t *handle,
    Uint16_t column,
    Uint16_t *data,
    Uint32_t length
)
{
    const Uint16_t command[] =
    {
        W25N01GVSFIG_CMD_READ_DATA,
        (Uint16_t)(column >> 8u),
        (Uint16_t)(column & 0xFFu),
        0u
    };
    W25N01GVSFIGStatus_t status;
    SpiStatus_t spiStatus;

    status = F_validateHandle(handle);
    if (status != W25N01GVSFIG_STATUS_OK)
    {
        return status;
    }
    if (data == NULL)
    {
        return W25N01GVSFIG_STATUS_NULL_PTR;
    }
    if ((length == 0u) ||
        (((Uint32_t)column + length) > W25N01GVSFIG_PAGE_TOTAL_SIZE))
    {
        return W25N01GVSFIG_STATUS_INVALID_LENGTH;
    }

    spiStatus = DSPI_select(handle->config->spiHandle,
                            handle->config->chipSelectGpio);
    if (spiStatus == SPI_STATUS_OK)
    {
        spiStatus = DSPI_exchange(handle->config->spiHandle,
                                  command,
                                  NULL,
                                  4u);
    }
    if (spiStatus == SPI_STATUS_OK)
    {
        spiStatus = DSPI_exchange(handle->config->spiHandle,
                                  NULL,
                                  data,
                                  length);
    }
    (void)DSPI_deselect(handle->config->spiHandle,
                        handle->config->chipSelectGpio);

    return F_fromSpiStatus(spiStatus);
}

/*
|===============================================================================
|
| Function:         F_programDataLoad
|
| Description:      Reset the internal program buffer to 0xFF and load the
|                   supplied data at the requested column.
|
| Dependencies:     WEL must be set before this command.
|
| Notes:
|
| Side Effects:     Replaces data in the internal program buffer.
|
| Return Value:     W25N01GVSFIGStatus_t indicating the load result.
|
|===============================================================================
| Variable        Access (I/O)    Description
|-------------------------------------------------------------------------------
| handle          I               Device instance handle
| column          I               Starting program-buffer column
| data            I               Source data buffer
| length          I               Number of bytes to load
|=============================================================================*/

static W25N01GVSFIGStatus_t F_programDataLoad
(
    W25N01GVSFIGHandle_t *handle,
    Uint16_t column,
    const Uint16_t *data,
    Uint32_t length
)
{
    const Uint16_t command[] =
    {
        W25N01GVSFIG_CMD_PROGRAM_DATA_LOAD,
        (Uint16_t)(column >> 8u),
        (Uint16_t)(column & 0xFFu)
    };
    W25N01GVSFIGStatus_t status;
    SpiStatus_t spiStatus;

    status = F_validateHandle(handle);
    if (status != W25N01GVSFIG_STATUS_OK)
    {
        return status;
    }
    if (data == NULL)
    {
        return W25N01GVSFIG_STATUS_NULL_PTR;
    }

    spiStatus = DSPI_select(handle->config->spiHandle,
                            handle->config->chipSelectGpio);
    if (spiStatus == SPI_STATUS_OK)
    {
        spiStatus = DSPI_exchange(handle->config->spiHandle,
                                  command,
                                  NULL,
                                  3u);
    }
    if (spiStatus == SPI_STATUS_OK)
    {
        spiStatus = DSPI_exchange(handle->config->spiHandle,
                                  data,
                                  NULL,
                                  length);
    }
    (void)DSPI_deselect(handle->config->spiHandle,
                        handle->config->chipSelectGpio);

    return F_fromSpiStatus(spiStatus);
}

/*
|===============================================================================
|
| Function:         F_validateProgramData
|
| Description:      Read the existing page data and confirm that programming
|                   the supplied bytes requires only one-to-zero transitions.
|
| Dependencies:     A valid initialized device handle.
|
| Notes:            Existing data is checked in small chunks to limit stack use.
|
| Side Effects:     Loads the existing NAND page into the internal data buffer.
|
| Return Value:     W25N01GVSFIG_STATUS_INVALID_BIT_TRANSITION when any zero
|                   bit would need to change back to one.
|
|===============================================================================
| Variable        Access (I/O)    Description
|-------------------------------------------------------------------------------
| handle          I/O             Device instance handle
| page            I               Page being validated
| column          I               Starting byte column
| data            I               Proposed program data
| length          I               Number of bytes to validate
|=============================================================================*/

static W25N01GVSFIGStatus_t F_validateProgramData
(
    W25N01GVSFIGHandle_t *handle,
    Uint32_t page,
    Uint16_t column,
    const Uint16_t *data,
    Uint32_t length
)
{
    W25N01GVSFIGStatus_t status;
    Uint16_t existingData[W25N01GVSFIG_VERIFY_BUFFER_SIZE];
    Uint16_t statusRegister;
    Uint16_t eccStatus;
    Uint16_t existingByte;
    Uint16_t newByte;
    Uint32_t checked;
    Uint32_t chunkLength;
    Uint32_t index;

    status = F_pageDataRead(handle, page, &statusRegister);
    if (status != W25N01GVSFIG_STATUS_OK)
    {
        return status;
    }

    eccStatus = statusRegister & W25N01GVSFIG_STATUS_ECC_MASK;
    if ((eccStatus != 0u) &&
        (eccStatus != W25N01GVSFIG_STATUS_ECC_CORRECTED_BITS))
    {
        return W25N01GVSFIG_STATUS_ECC_UNCORRECTABLE;
    }

    for (checked = 0u; checked < length; checked += chunkLength)
    {
        chunkLength = length - checked;
        if (chunkLength > W25N01GVSFIG_VERIFY_BUFFER_SIZE)
        {
            chunkLength = W25N01GVSFIG_VERIFY_BUFFER_SIZE;
        }

        status = F_readData(handle,
                            (Uint16_t)((Uint32_t)column + checked),
                            existingData,
                            chunkLength);
        if (status != W25N01GVSFIG_STATUS_OK)
        {
            return status;
        }

        for (index = 0u; index < chunkLength; index++)
        {
            existingByte = existingData[index] & 0xFFu;
            newByte = data[checked + index] & 0xFFu;
            if ((existingByte & newByte) != newByte)
            {
                return W25N01GVSFIG_STATUS_INVALID_BIT_TRANSITION;
            }
        }
    }

    return W25N01GVSFIG_STATUS_OK;
}

/*
|===============================================================================
|
| Function:         F_isWriteProtected
|
| Description:      Determine whether any block-protection bits are active.
|
| Dependencies:     A valid initialized device handle.
|
| Notes:
|
| Side Effects:     Reads the protection feature register.
|
| Return Value:     W25N01GVSFIGStatus_t indicating the register read result.
|
|===============================================================================
| Variable          Access (I/O)    Description
|-------------------------------------------------------------------------------
| handle            I               Device instance handle
| isWriteProtected  O               TRUE when block protection is active
|=============================================================================*/

static W25N01GVSFIGStatus_t F_isWriteProtected
(
    W25N01GVSFIGHandle_t *handle,
    Bool_t *isWriteProtected
)
{
    W25N01GVSFIGStatus_t status;
    Uint16_t protection;

    if (isWriteProtected == NULL)
    {
        return W25N01GVSFIG_STATUS_NULL_PTR;
    }

    status = F_getFeature(handle,
                          W25N01GVSFIG_FEATURE_PROTECTION,
                          &protection);
    if (status == W25N01GVSFIG_STATUS_OK)
    {
        *isWriteProtected =
            ((protection & W25N01GVSFIG_PROTECTION_BLOCK_MASK) != 0u);
    }

    return status;
}

/*
|===============================================================================
|
| Function:         F_fromSpiStatus
|
| Description:      Convert a BSW SPI status into a W25N01GVSFIG status.
|
| Dependencies:
|
| Notes:
|
| Side Effects:     None.
|
| Return Value:     W25N01GVSFIG_STATUS_OK or
|                   W25N01GVSFIG_STATUS_SPI_ERROR.
|
|===============================================================================
| Variable        Access (I/O)    Description
|-------------------------------------------------------------------------------
| status          I               BSW SPI operation status
|=============================================================================*/

static W25N01GVSFIGStatus_t F_fromSpiStatus
(
    SpiStatus_t status
)
{
    return (status == SPI_STATUS_OK) ?
           W25N01GVSFIG_STATUS_OK :
           W25N01GVSFIG_STATUS_SPI_ERROR;
}

/*=== End of File ============================================================*/
