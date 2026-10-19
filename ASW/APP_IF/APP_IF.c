#include "APP_IF.h"

#define APP_IF_STATUS_UNKNOWN_COMMAND (0x80u)
#define APP_IF_STATUS_BAD_LENGTH      (0x81u)
#define APP_IF_STATUS_BAD_CRC         (0x82u)

typedef enum ReceiveState_t
{
    RX_START,
    RX_START_SECOND,
    RX_COMMAND,
    RX_LENGTH_LOW,
    RX_LENGTH_HIGH,
    RX_PAYLOAD,
    RX_CRC_LOW,
    RX_CRC_HIGH,
    RX_DISCARD
} ReceiveState_t;

static W25N01GVSFIGHandle_t *F_nand;
static AppIfSend_t F_send;
static ReceiveState_t F_state;
static Uint16_t F_command;
static Uint16_t F_length;
static Uint16_t F_received;
static Uint16_t F_crc;
static Uint16_t F_receivedCrc;
static Uint16_t F_payload[APP_IF_MAX_PAYLOAD];
static Uint16_t F_response[W25N01GVSFIG_PAGE_DATA_SIZE + 1u];

static Uint16_t F_crcOctet(Uint16_t crc, Uint16_t octet)
{
    Uint16_t bit;
    crc ^= (Uint16_t)(octet << 8u);
    for (bit = 0u; bit < 8u; bit++)
    {
        crc = (Uint16_t)((crc & 0x8000u) != 0u ?
                        (crc << 1u) ^ 0x1021u : crc << 1u);
    }
    return crc;
}

static Uint16_t F_get16(const Uint16_t *data)
{
    return (Uint16_t)(data[0u] | (data[1u] << 8u));
}

static Uint32_t F_get32(const Uint16_t *data)
{
    return (Uint32_t)F_get16(data) |
           ((Uint32_t)F_get16(&data[2u]) << 16u);
}

static void F_put16(Uint16_t *data, Uint16_t value)
{
    data[0u] = value & 0xFFu;
    data[1u] = (value >> 8u) & 0xFFu;
}

static void F_put32(Uint16_t *data, Uint32_t value)
{
    F_put16(data, (Uint16_t)value);
    F_put16(&data[2u], (Uint16_t)(value >> 16u));
}

static void F_respond(Uint16_t status, Uint16_t dataLength)
{
    Uint16_t length = dataLength + 1u;
    Uint16_t header[3u];
    Uint16_t crc = 0xFFFFu;
    Uint16_t index;

    F_response[0u] = status;
    header[0u] = F_command;
    F_put16(&header[1u], length);
    if ((F_send == NULL) || !F_send(0xA5u) || !F_send(0x5Au))
    {
        return;
    }
    for (index = 0u; index < 3u; index++)
    {
        crc = F_crcOctet(crc, header[index]);
        if (!F_send(header[index]))
        {
            return;
        }
    }
    for (index = 0u; index < length; index++)
    {
        crc = F_crcOctet(crc, F_response[index]);
        if (!F_send(F_response[index]))
        {
            return;
        }
    }
    if (F_send(crc & 0xFFu))
    {
        (void)F_send((crc >> 8u) & 0xFFu);
    }
}

static void F_execute(void)
{
    W25N01GVSFIGStatus_t status;
    W25N01GVSFIGId_t id;
    Bool_t badBlock = FALSE;
    Uint16_t length;

    switch (F_command)
    {
        case APP_IF_CMD_INFO:
            if (F_length != 0u)
            {
                break;
            }
            status = W25N01GVSFIG_readId(F_nand, &id);
            if (status == W25N01GVSFIG_STATUS_OK)
            {
                F_response[1u] = 1u;
                F_response[2u] = id.manufacturer;
                F_response[3u] = id.deviceHigh;
                F_response[4u] = id.deviceLow;
                F_put16(&F_response[5u], W25N01GVSFIG_PAGE_DATA_SIZE);
                F_put16(&F_response[7u], W25N01GVSFIG_PAGE_SPARE_SIZE);
                F_put16(&F_response[9u], W25N01GVSFIG_PAGES_PER_BLOCK);
                F_put16(&F_response[11u], W25N01GVSFIG_BLOCK_COUNT);
                F_put32(&F_response[13u], W25N01GVSFIG_PAGE_COUNT);
            }
            F_respond((Uint16_t)status,
                      status == W25N01GVSFIG_STATUS_OK ? 16u : 0u);
            return;

        case APP_IF_CMD_READ_PAGE:
            if (F_length != 8u)
            {
                break;
            }
            length = F_get16(&F_payload[6u]);
            if ((length == 0u) || (length > W25N01GVSFIG_PAGE_DATA_SIZE))
            {
                F_respond(W25N01GVSFIG_STATUS_INVALID_LENGTH, 0u);
                return;
            }
            status = W25N01GVSFIG_readPage(F_nand, F_get32(F_payload),
                                         F_get16(&F_payload[4u]),
                                         &F_response[1u], length);
            F_respond((Uint16_t)status,
                      (status == W25N01GVSFIG_STATUS_OK ||
                       status == W25N01GVSFIG_STATUS_ECC_CORRECTED) ? length : 0u);
            return;

        case APP_IF_CMD_PROGRAM_PAGE:
            if (F_length <= 6u)
            {
                break;
            }
            status = W25N01GVSFIG_programPage(F_nand, F_get32(F_payload),
                                            F_get16(&F_payload[4u]),
                                            &F_payload[6u], F_length - 6u);
            F_respond((Uint16_t)status, 0u);
            return;

        case APP_IF_CMD_ERASE_BLOCK:
        case APP_IF_CMD_IS_BAD_BLOCK:
            if (F_length != 2u)
            {
                break;
            }
            if (F_command == APP_IF_CMD_ERASE_BLOCK)
            {
                status = W25N01GVSFIG_eraseBlock(F_nand, F_get16(F_payload));
                F_respond((Uint16_t)status, 0u);
            }
            else
            {
                status = W25N01GVSFIG_isBadBlock(F_nand, F_get16(F_payload),
                                               &badBlock);
                F_response[1u] = badBlock ? 1u : 0u;
                F_respond((Uint16_t)status,
                          status == W25N01GVSFIG_STATUS_OK ? 1u : 0u);
            }
            return;

        default:
            F_respond(APP_IF_STATUS_UNKNOWN_COMMAND, 0u);
            return;
    }
    F_respond(APP_IF_STATUS_BAD_LENGTH, 0u);
}

void APP_IF_reset(void)
{
    F_state = RX_START;
    F_length = 0u;
    F_received = 0u;
    F_crc = 0xFFFFu;
}

void APP_IF_init(W25N01GVSFIGHandle_t *nand, AppIfSend_t send)
{
    F_nand = nand;
    F_send = send;
    APP_IF_reset();
}

void APP_IF_receive(Uint16_t octet)
{
    octet &= 0xFFu;

    switch (F_state)
    {
        case RX_START:
            if (octet == 0xA5u)
            {
                F_state = RX_START_SECOND;
            }
            return;
        case RX_START_SECOND:
            F_state = octet == 0x5Au ? RX_COMMAND :
                      (octet == 0xA5u ? RX_START_SECOND : RX_START);
            return;
        case RX_COMMAND:
            F_command = octet;
            F_crc = F_crcOctet(0xFFFFu, octet);
            F_state = RX_LENGTH_LOW;
            return;
        case RX_LENGTH_LOW:
            F_length = octet;
            F_state = RX_LENGTH_HIGH;
            break;
        case RX_LENGTH_HIGH:
            F_length |= (Uint16_t)(octet << 8u);
            if (F_length > APP_IF_MAX_PAYLOAD)
            {
                /* Ignore the rest until reset; it may contain START bytes. */
                F_state = RX_DISCARD;
                return;
            }
            F_received = 0u;
            F_state = F_length == 0u ? RX_CRC_LOW : RX_PAYLOAD;
            break;
        case RX_PAYLOAD:
            F_payload[F_received++] = octet;
            if (F_received == F_length)
            {
                F_state = RX_CRC_LOW;
            }
            break;
        case RX_CRC_LOW:
            F_receivedCrc = octet;
            F_state = RX_CRC_HIGH;
            return;
        case RX_CRC_HIGH:
            F_receivedCrc |= (Uint16_t)(octet << 8u);
            F_state = RX_START;
            if (F_receivedCrc == F_crc)
            {
                F_execute();
            }
            else
            {
                F_respond(APP_IF_STATUS_BAD_CRC, 0u);
            }
            return;
        case RX_DISCARD:
        default:
            return;
    }
    F_crc = F_crcOctet(F_crc, octet);
}
