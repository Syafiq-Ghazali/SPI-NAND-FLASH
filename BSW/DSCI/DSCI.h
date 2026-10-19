#ifndef DSCI_H
#define DSCI_H

#include "DATATYPE.h"
#include "DGPIO.h"
#include "driverlib.h"

typedef struct SciConfig_t
{
    Uint32_t base;
    Uint32_t lspclkHz;
    Uint32_t baud;
    Uint32_t timerBase;
    Uint16_t timerPrescaler;
    Uint32_t txTimeoutUs;
    const GpioConfig_t *rxGpio;
    const GpioConfig_t *txGpio;
} SciConfig_t;

typedef enum SciRxStatus_t
{
    SCI_RX_EMPTY,
    SCI_RX_OK,
    SCI_RX_ERROR
} SciRxStatus_t;

/* The configured timer must run at 1 MHz and is owned by this transport. */
void DSCI_init(const SciConfig_t *config);
Uint32_t DSCI_timeUs(const SciConfig_t *config);
SciRxStatus_t DSCI_receive(const SciConfig_t *config, Uint16_t *octet);
Bool_t DSCI_send(const SciConfig_t *config, Uint16_t octet);

#endif
