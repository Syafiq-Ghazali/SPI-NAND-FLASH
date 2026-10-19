#include "DSCI.h"

Bool_t DSCI_receive(const SciConfig_t *config, Uint16_t *octet, Bool_t *received)
{
    (void)config;
    (void)octet;
    *received = FALSE;
    return TRUE;
}

Bool_t DSCI_send(const SciConfig_t *config, Uint16_t octet)
{
    (void)config;
    (void)octet;
    return TRUE;
}
