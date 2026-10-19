#ifndef APP_IF_H
#define APP_IF_H

#include "W25N01GVSFIG.h"

#define APP_IF_MAX_PAYLOAD       (2054u)
#define APP_IF_RX_TIMEOUT_US     (1000000uL)

typedef enum AppIfCommand_t
{
    APP_IF_CMD_INFO = 1u,
    APP_IF_CMD_READ_PAGE = 2u,
    APP_IF_CMD_PROGRAM_PAGE = 3u,
    APP_IF_CMD_ERASE_BLOCK = 4u,
    APP_IF_CMD_IS_BAD_BLOCK = 5u
} AppIfCommand_t;

/* A failed send aborts the response. Only the low eight bits are transmitted. */
typedef Bool_t (*AppIfSend_t)(Uint16_t octet);

void APP_IF_init(W25N01GVSFIGHandle_t *nand, AppIfSend_t send);
void APP_IF_receive(Uint16_t octet, Uint32_t nowUs);
void APP_IF_poll(Uint32_t nowUs);
void APP_IF_reset(void);

#endif
