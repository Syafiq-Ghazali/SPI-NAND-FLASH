#include "DSCI.h"

void DSCI_init(const SciConfig_t *config)
{
    DGPIO_init(config->rxGpio);
    DGPIO_init(config->txGpio);
    CPUTimer_stopTimer(config->timerBase);
    CPUTimer_setPreScaler(config->timerBase, config->timerPrescaler);
    CPUTimer_setPeriod(config->timerBase, 0xFFFFFFFFuL);
    CPUTimer_disableInterrupt(config->timerBase);
    CPUTimer_startTimer(config->timerBase);

    SCI_disableModule(config->base);
    SCI_disableFIFO(config->base);
    SCI_setConfig(config->base, config->lspclkHz, config->baud,
                  SCI_CONFIG_WLEN_8 | SCI_CONFIG_STOP_ONE | SCI_CONFIG_PAR_NONE);
    SCI_disableInterrupt(config->base, SCI_INT_RXRDY_BRKDT | SCI_INT_TXRDY);
    SCI_enableModule(config->base);
    SCI_performSoftwareReset(config->base);
}

Uint32_t DSCI_timeUs(const SciConfig_t *config)
{
    return 0xFFFFFFFFuL - CPUTimer_getTimerCount(config->timerBase);
}

SciRxStatus_t DSCI_receive(const SciConfig_t *config, Uint16_t *octet)
{
    if ((SCI_getRxStatus(config->base) & SCI_RXSTATUS_ERROR) != 0u)
    {
        SCI_performSoftwareReset(config->base);
        return SCI_RX_ERROR;
    }
    if (!SCI_isDataAvailableNonFIFO(config->base))
    {
        return SCI_RX_EMPTY;
    }
    *octet = SCI_readCharNonBlocking(config->base) & 0xFFu;
    return SCI_RX_OK;
}

Bool_t DSCI_send(const SciConfig_t *config, Uint16_t octet)
{
    Uint32_t start = DSCI_timeUs(config);
    while (!SCI_isSpaceAvailableNonFIFO(config->base))
    {
        if ((Uint32_t)(DSCI_timeUs(config) - start) >= config->txTimeoutUs)
        {
            return FALSE;
        }
    }
    SCI_writeCharNonBlocking(config->base, octet & 0xFFu);
    return TRUE;
}
