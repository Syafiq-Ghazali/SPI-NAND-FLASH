#include "APP_IF.h"

#ifdef _WIN32
#define TEST_API __declspec(dllexport)
#else
#define TEST_API
#endif

static W25N01GVSFIGHandle_t F_handle;
static Uint16_t F_output[8192u];
static Uint16_t F_outputLength;
static Uint16_t F_sendLimit;
static Uint16_t F_status;
static Uint16_t F_calls;
static Uint32_t F_arguments[4u];

static Bool_t F_capture(Uint16_t octet)
{
    if (F_outputLength >= F_sendLimit)
    {
        return FALSE;
    }
    F_output[F_outputLength++] = octet;
    return TRUE;
}

TEST_API void test_init(void)
{
    Uint16_t index;
    F_outputLength = 0u;
    F_sendLimit = 8192u;
    F_calls = 0u;
    F_status = 0u;
    for (index = 0u; index < 4u; index++)
    {
        F_arguments[index] = 0u;
    }
    APP_IF_init(&F_handle, F_capture);
}

TEST_API void test_receive(Uint16_t octet, Uint32_t nowUs)
{
    APP_IF_receive(octet, nowUs);
}

TEST_API void test_poll(Uint32_t nowUs) { APP_IF_poll(nowUs); }
TEST_API void test_status(Uint16_t status) { F_status = status; }
TEST_API void test_send_limit(Uint16_t limit) { F_sendLimit = limit; }
TEST_API Uint16_t test_calls(void) { return F_calls; }
TEST_API Uint16_t test_output_length(void) { return F_outputLength; }
TEST_API Uint16_t test_output(Uint16_t index) { return F_output[index]; }
TEST_API Uint32_t test_argument(Uint16_t index) { return F_arguments[index]; }

W25N01GVSFIGStatus_t W25N01GVSFIG_readId(W25N01GVSFIGHandle_t *handle,
                                       W25N01GVSFIGId_t *id)
{
    (void)handle;
    F_calls++;
    id->manufacturer = 0xEFu;
    id->deviceHigh = 0xAAu;
    id->deviceLow = 0x21u;
    return (W25N01GVSFIGStatus_t)F_status;
}

W25N01GVSFIGStatus_t W25N01GVSFIG_readPage(W25N01GVSFIGHandle_t *handle,
    Uint32_t page, Uint16_t offset, Uint16_t *data, Uint32_t length)
{
    Uint32_t index;
    (void)handle;
    F_calls++;
    F_arguments[0u] = page;
    F_arguments[1u] = offset;
    F_arguments[2u] = length;
    for (index = 0u; index < length; index++)
    {
        data[index] = (Uint16_t)(index & 0xFFu);
    }
    return (W25N01GVSFIGStatus_t)F_status;
}

W25N01GVSFIGStatus_t W25N01GVSFIG_programPage(W25N01GVSFIGHandle_t *handle,
    Uint32_t page, Uint16_t offset, const Uint16_t *data, Uint32_t length)
{
    Uint32_t index;
    (void)handle;
    F_calls++;
    F_arguments[0u] = page;
    F_arguments[1u] = offset;
    F_arguments[2u] = length;
    for (index = 0u; index < length; index++)
    {
        F_arguments[3u] += data[index];
    }
    return (W25N01GVSFIGStatus_t)F_status;
}

W25N01GVSFIGStatus_t W25N01GVSFIG_eraseBlock(W25N01GVSFIGHandle_t *handle,
                                          Uint16_t block)
{
    (void)handle;
    F_calls++;
    F_arguments[0u] = block;
    return (W25N01GVSFIGStatus_t)F_status;
}

W25N01GVSFIGStatus_t W25N01GVSFIG_isBadBlock(W25N01GVSFIGHandle_t *handle,
                                         Uint16_t block, Bool_t *bad)
{
    (void)handle;
    F_calls++;
    F_arguments[0u] = block;
    *bad = TRUE;
    return (W25N01GVSFIGStatus_t)F_status;
}
