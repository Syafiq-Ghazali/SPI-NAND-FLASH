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
    const GpioConfig_t *rxGpio;
    const GpioConfig_t *txGpio;
} SciConfig_t;

/* UART interface stubs; no hardware is accessed. */
/* TRUE means success; received distinguishes an octet from an idle receiver. */
Bool_t DSCI_receive(const SciConfig_t *config, Uint16_t *octet, Bool_t *received);
Bool_t DSCI_send(const SciConfig_t *config, Uint16_t octet);

#endif
