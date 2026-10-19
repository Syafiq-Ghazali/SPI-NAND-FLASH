#include "BRD.h"
#include "APP_IF.h"

static Bool_t F_send(Uint16_t octet)
{
    return DSCI_send(&BRD_SCI_CONFIG, octet);
}

void main
(
    void
)
{
    Uint16_t octet;
    SciRxStatus_t rxStatus;

    BRD_init();
    /* Failed initialization invalidates the handle; commands then report it. */
    (void)W25N01GVSFIG_init(&BRD_W25N01GVSFIG_HANDLE,
                           &BRD_W25N01GVSFIG_CONFIG);
    APP_IF_init(&BRD_W25N01GVSFIG_HANDLE, F_send);

    for (;;)
    {
        rxStatus = DSCI_receive(&BRD_SCI_CONFIG, &octet);
        if (rxStatus == SCI_RX_OK)
        {
            APP_IF_receive(octet, DSCI_timeUs(&BRD_SCI_CONFIG));
        }
        else if (rxStatus == SCI_RX_ERROR)
        {
            APP_IF_reset();
        }
        APP_IF_poll(DSCI_timeUs(&BRD_SCI_CONFIG));
    }
}
