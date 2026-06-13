/*
|===============================================================================
|
| File:         DSPI.c
|
| Project:      SPI_NAND
|
| Processor:    TI TMS320F28003x
|
| Description:  Blocking SPI driver implementation.
|
|=============================================================================*/

/*=== INCLUDE FILES ==========================================================*/

#include "DSPI.h"

/*=== FUNCTION DEFINITIONS ===================================================*/

SpiStatus_t DSPI_init
(
    SpiHandle_t *handle,
    const SpiConfig_t *config
)
{
    if ((handle == NULL) || (config == NULL) ||
        (config->simoGpioConfig == NULL) ||
        (config->somiGpioConfig == NULL) ||
        (config->clkGpioConfig == NULL))
    {
        return SPI_STATUS_NULL_PTR;
    }

    if ((config->dataWidth == 0u) || (config->dataWidth > 16u))
    {
        return SPI_STATUS_INVALID_CONFIG;
    }

    DGPIO_init(config->simoGpioConfig);
    DGPIO_init(config->somiGpioConfig);
    DGPIO_init(config->clkGpioConfig);

    SPI_disableModule(config->base);
    SPI_reset(config->base);
    SPI_disableFIFO(config->base);
    SPI_disableLoopback(config->base);
    SPI_setEmulationMode(config->base, SPI_EMULATION_STOP_AFTER_TRANSMIT);
    SPI_setConfig(config->base,
                  config->sclkFrequency,
                  config->transferProtocol,
                  config->mode,
                  config->bitRate,
                  config->dataWidth);
    SPI_enableModule(config->base);

    handle->config = config;

    return SPI_STATUS_OK;
}

SpiStatus_t DSPI_select
(
    SpiHandle_t *handle,
    Uint32_t chipSelectGpio
)
{
    if (handle == NULL)
    {
        return SPI_STATUS_NULL_PTR;
    }

    if (handle->config == NULL)
    {
        return SPI_STATUS_NOT_INITIALIZED;
    }

    DGPIO_pinWrite(chipSelectGpio, GPIO_STATE_OFF);
    return SPI_STATUS_OK;
}

SpiStatus_t DSPI_deselect
(
    SpiHandle_t *handle,
    Uint32_t chipSelectGpio
)
{
    if (handle == NULL)
    {
        return SPI_STATUS_NULL_PTR;
    }

    if (handle->config == NULL)
    {
        return SPI_STATUS_NOT_INITIALIZED;
    }

    while (SPI_isBusy(handle->config->base))
    {
    }

    DGPIO_pinWrite(chipSelectGpio, GPIO_STATE_ON);
    return SPI_STATUS_OK;
}

SpiStatus_t DSPI_exchange
(
    SpiHandle_t *handle,
    const Uint16_t *txData,
    Uint16_t *rxData,
    Uint32_t length
)
{
    Uint32_t i;
    Uint16_t txWord;
    Uint16_t rxWord;
    Uint16_t shift;

    if (handle == NULL)
    {
        return SPI_STATUS_NULL_PTR;
    }

    if (handle->config == NULL)
    {
        return SPI_STATUS_NOT_INITIALIZED;
    }

    shift = (Uint16_t)(16u - handle->config->dataWidth);

    for (i = 0u; i < length; i++)
    {
        txWord = (txData == NULL) ? DSPI_DUMMY_DATA : txData[i];

        SPI_writeDataBlockingNonFIFO(handle->config->base,
                                     (Uint16_t)(txWord << shift));
        rxWord = SPI_readDataBlockingNonFIFO(handle->config->base);

        if (rxData != NULL)
        {
            rxData[i] = rxWord;
        }
    }

    return SPI_STATUS_OK;
}

SpiStatus_t DSPI_transfer
(
    SpiHandle_t *handle,
    const Uint16_t *txData,
    Uint16_t *rxData,
    Uint32_t length,
    Uint32_t chipSelectGpio
)
{
    SpiStatus_t status;

    status = DSPI_select(handle, chipSelectGpio);
    if (status != SPI_STATUS_OK)
    {
        return status;
    }

    status = DSPI_exchange(handle, txData, rxData, length);
    (void)DSPI_deselect(handle, chipSelectGpio);

    return status;
}

/*=== End of File ============================================================*/
