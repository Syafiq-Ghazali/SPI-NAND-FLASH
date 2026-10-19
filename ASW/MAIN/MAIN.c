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
    Bool_t received;

    BRD_init();
    /* Failed initialization invalidates the handle; commands then report it. */
    (void)W25N01GVSFIG_init(&BRD_W25N01GVSFIG_HANDLE,
                           &BRD_W25N01GVSFIG_CONFIG);
    APP_IF_init(&BRD_W25N01GVSFIG_HANDLE, F_send);

    for (;;)
    {
        if (!DSCI_receive(&BRD_SCI_CONFIG, &octet, &received))
        {
            APP_IF_reset();
        }
        else if (received)
        {
            APP_IF_receive(octet);
        }
    }
}
