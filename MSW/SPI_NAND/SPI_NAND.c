/*
|===============================================================================
|
| File:         SPI_NAND.c
|
| Project:      SPI_NAND
|
| Processor:    TI TMS320F28003x
|
| Description:  Vendor-neutral SPI NAND command implementation.
|
|=============================================================================*/

/*=== INCLUDE FILES ==========================================================*/

#include "SPI_NAND.h"

/*=== PRIVATE FUNCTION PROTOTYPES ============================================*/

static SpiNandStatus_t F_validateHandle(const SpiNandHandle_t *handle);
static SpiNandStatus_t F_command(SpiNandHandle_t *handle,
                                const Uint16_t *command,
                                Uint32_t commandLength);
static SpiNandStatus_t F_rowCommand(SpiNandHandle_t *handle,
                                   Uint16_t command,
                                   Uint32_t rowAddress);
static SpiNandStatus_t F_fromSpiStatus(SpiStatus_t status);

/*=== FUNCTION DEFINITIONS ===================================================*/

SpiNandStatus_t SPI_NAND_init
(
    SpiNandHandle_t *handle,
    const SpiNandConfig_t *config
)
{
    if ((handle == NULL) || (config == NULL) || (config->spiHandle == NULL))
    {
        return SPI_NAND_STATUS_NULL_PTR;
    }

    if (config->spiHandle->config == NULL)
    {
        return SPI_NAND_STATUS_NOT_INITIALIZED;
    }

    handle->config = config;
    handle->statusRegister = 0u;

    return SPI_NAND_STATUS_OK;
}

SpiNandStatus_t SPI_NAND_reset
(
    SpiNandHandle_t *handle
)
{
    const Uint16_t command[] = {SPI_NAND_CMD_RESET};
    SpiNandStatus_t status;

    status = F_command(handle, command, 1u);
    if (status != SPI_NAND_STATUS_OK)
    {
        return status;
    }

    return SPI_NAND_waitReady(handle);
}

SpiNandStatus_t SPI_NAND_readId
(
    SpiNandHandle_t *handle,
    Uint16_t *id,
    Uint32_t length
)
{
    const Uint16_t command[] = {SPI_NAND_CMD_READ_ID};
    Uint32_t dummyIndex;
    SpiNandStatus_t status;
    SpiStatus_t spiStatus;

    status = F_validateHandle(handle);
    if (status != SPI_NAND_STATUS_OK)
    {
        return status;
    }

    if (id == NULL)
    {
        return SPI_NAND_STATUS_NULL_PTR;
    }

    spiStatus = DSPI_select(handle->config->spiHandle,
                            handle->config->chipSelectGpio);
    if (spiStatus == SPI_STATUS_OK)
    {
        spiStatus = DSPI_exchange(handle->config->spiHandle, command, NULL, 1u);
    }
    if (spiStatus == SPI_STATUS_OK)
    {
        for (dummyIndex = 0u;
             (dummyIndex < handle->config->readIdDummyBytes) &&
             (spiStatus == SPI_STATUS_OK);
             dummyIndex++)
        {
            spiStatus = DSPI_exchange(handle->config->spiHandle,
                                      &handle->config->readIdDummyValue,
                                      NULL,
                                      1u);
        }
    }
    if (spiStatus == SPI_STATUS_OK)
    {
        spiStatus = DSPI_exchange(handle->config->spiHandle, NULL, id, length);
    }

    (void)DSPI_deselect(handle->config->spiHandle,
                        handle->config->chipSelectGpio);

    return F_fromSpiStatus(spiStatus);
}

SpiNandStatus_t SPI_NAND_getFeature
(
    SpiNandHandle_t *handle,
    Uint16_t featureAddress,
    Uint16_t *featureValue
)
{
    const Uint16_t command[] =
    {
        SPI_NAND_CMD_GET_FEATURE,
        featureAddress
    };
    SpiNandStatus_t status;
    SpiStatus_t spiStatus;

    status = F_validateHandle(handle);
    if (status != SPI_NAND_STATUS_OK)
    {
        return status;
    }

    if (featureValue == NULL)
    {
        return SPI_NAND_STATUS_NULL_PTR;
    }

    spiStatus = DSPI_select(handle->config->spiHandle,
                            handle->config->chipSelectGpio);
    if (spiStatus == SPI_STATUS_OK)
    {
        spiStatus = DSPI_exchange(handle->config->spiHandle, command, NULL, 2u);
    }
    if (spiStatus == SPI_STATUS_OK)
    {
        spiStatus = DSPI_exchange(handle->config->spiHandle, NULL,
                                  featureValue, 1u);
    }

    (void)DSPI_deselect(handle->config->spiHandle,
                        handle->config->chipSelectGpio);

    return F_fromSpiStatus(spiStatus);
}

SpiNandStatus_t SPI_NAND_setFeature
(
    SpiNandHandle_t *handle,
    Uint16_t featureAddress,
    Uint16_t featureValue
)
{
    const Uint16_t command[] =
    {
        SPI_NAND_CMD_SET_FEATURE,
        featureAddress,
        featureValue
    };

    return F_command(handle, command, 3u);
}

SpiNandStatus_t SPI_NAND_writeEnable
(
    SpiNandHandle_t *handle
)
{
    const Uint16_t command[] = {SPI_NAND_CMD_WRITE_ENABLE};

    return F_command(handle, command, 1u);
}

SpiNandStatus_t SPI_NAND_waitReady
(
    SpiNandHandle_t *handle
)
{
    Uint32_t poll;
    Uint16_t statusRegister;
    SpiNandStatus_t status;

    status = F_validateHandle(handle);
    if (status != SPI_NAND_STATUS_OK)
    {
        return status;
    }

    for (poll = 0u; poll < handle->config->readyPollLimit; poll++)
    {
        status = SPI_NAND_getFeature(handle,
                                     SPI_NAND_FEATURE_STATUS,
                                     &statusRegister);
        if (status != SPI_NAND_STATUS_OK)
        {
            return status;
        }

        handle->statusRegister = statusRegister;
        if ((statusRegister & handle->config->busyMask) == 0u)
        {
            return SPI_NAND_STATUS_OK;
        }
    }

    return SPI_NAND_STATUS_TIMEOUT;
}

SpiNandStatus_t SPI_NAND_pageRead
(
    SpiNandHandle_t *handle,
    Uint32_t rowAddress
)
{
    SpiNandStatus_t status;

    status = F_rowCommand(handle, SPI_NAND_CMD_PAGE_READ, rowAddress);
    if (status != SPI_NAND_STATUS_OK)
    {
        return status;
    }

    return SPI_NAND_waitReady(handle);
}

SpiNandStatus_t SPI_NAND_readCache
(
    SpiNandHandle_t *handle,
    Uint16_t columnAddress,
    Uint16_t *data,
    Uint32_t length
)
{
    const Uint16_t command[] =
    {
        SPI_NAND_CMD_READ_CACHE,
        (Uint16_t)(columnAddress >> 8u),
        (Uint16_t)(columnAddress & 0xFFu),
        0u
    };
    SpiNandStatus_t status;
    SpiStatus_t spiStatus;

    status = F_validateHandle(handle);
    if (status != SPI_NAND_STATUS_OK)
    {
        return status;
    }

    if (data == NULL)
    {
        return SPI_NAND_STATUS_NULL_PTR;
    }

    spiStatus = DSPI_select(handle->config->spiHandle,
                            handle->config->chipSelectGpio);
    if (spiStatus == SPI_STATUS_OK)
    {
        spiStatus = DSPI_exchange(handle->config->spiHandle, command, NULL, 4u);
    }
    if (spiStatus == SPI_STATUS_OK)
    {
        spiStatus = DSPI_exchange(handle->config->spiHandle, NULL, data, length);
    }

    (void)DSPI_deselect(handle->config->spiHandle,
                        handle->config->chipSelectGpio);

    return F_fromSpiStatus(spiStatus);
}

SpiNandStatus_t SPI_NAND_programLoad
(
    SpiNandHandle_t *handle,
    Uint16_t columnAddress,
    const Uint16_t *data,
    Uint32_t length
)
{
    const Uint16_t command[] =
    {
        SPI_NAND_CMD_PROGRAM_LOAD,
        (Uint16_t)(columnAddress >> 8u),
        (Uint16_t)(columnAddress & 0xFFu)
    };
    SpiNandStatus_t status;
    SpiStatus_t spiStatus;

    status = F_validateHandle(handle);
    if (status != SPI_NAND_STATUS_OK)
    {
        return status;
    }

    if (data == NULL)
    {
        return SPI_NAND_STATUS_NULL_PTR;
    }

    spiStatus = DSPI_select(handle->config->spiHandle,
                            handle->config->chipSelectGpio);
    if (spiStatus == SPI_STATUS_OK)
    {
        spiStatus = DSPI_exchange(handle->config->spiHandle, command, NULL, 3u);
    }
    if (spiStatus == SPI_STATUS_OK)
    {
        spiStatus = DSPI_exchange(handle->config->spiHandle, data, NULL, length);
    }

    (void)DSPI_deselect(handle->config->spiHandle,
                        handle->config->chipSelectGpio);

    return F_fromSpiStatus(spiStatus);
}

SpiNandStatus_t SPI_NAND_programExecute
(
    SpiNandHandle_t *handle,
    Uint32_t rowAddress
)
{
    SpiNandStatus_t status;

    status = F_rowCommand(handle, SPI_NAND_CMD_PROGRAM_EXECUTE, rowAddress);
    if (status != SPI_NAND_STATUS_OK)
    {
        return status;
    }

    return SPI_NAND_waitReady(handle);
}

SpiNandStatus_t SPI_NAND_blockErase
(
    SpiNandHandle_t *handle,
    Uint32_t rowAddress
)
{
    SpiNandStatus_t status;

    status = F_rowCommand(handle, SPI_NAND_CMD_BLOCK_ERASE, rowAddress);
    if (status != SPI_NAND_STATUS_OK)
    {
        return status;
    }

    return SPI_NAND_waitReady(handle);
}

static SpiNandStatus_t F_validateHandle
(
    const SpiNandHandle_t *handle
)
{
    if (handle == NULL)
    {
        return SPI_NAND_STATUS_NULL_PTR;
    }

    if ((handle->config == NULL) || (handle->config->spiHandle == NULL) ||
        (handle->config->spiHandle->config == NULL))
    {
        return SPI_NAND_STATUS_NOT_INITIALIZED;
    }

    return SPI_NAND_STATUS_OK;
}

static SpiNandStatus_t F_command
(
    SpiNandHandle_t *handle,
    const Uint16_t *command,
    Uint32_t commandLength
)
{
    SpiNandStatus_t status;

    status = F_validateHandle(handle);
    if (status != SPI_NAND_STATUS_OK)
    {
        return status;
    }

    if (command == NULL)
    {
        return SPI_NAND_STATUS_NULL_PTR;
    }

    return F_fromSpiStatus(
        DSPI_transfer(handle->config->spiHandle,
                      command,
                      NULL,
                      commandLength,
                      handle->config->chipSelectGpio));
}

static SpiNandStatus_t F_rowCommand
(
    SpiNandHandle_t *handle,
    Uint16_t command,
    Uint32_t rowAddress
)
{
    const Uint16_t commandBuffer[] =
    {
        command,
        (Uint16_t)((rowAddress >> 16u) & 0xFFu),
        (Uint16_t)((rowAddress >> 8u) & 0xFFu),
        (Uint16_t)(rowAddress & 0xFFu)
    };

    return F_command(handle, commandBuffer, 4u);
}

static SpiNandStatus_t F_fromSpiStatus
(
    SpiStatus_t status
)
{
    return (status == SPI_STATUS_OK) ?
           SPI_NAND_STATUS_OK :
           SPI_NAND_STATUS_SPI_ERROR;
}

/*=== End of File ============================================================*/
