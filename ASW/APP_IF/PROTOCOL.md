# UART protocol v1

Shared message format for the MCU and future Python client.
UART receive/send are currently stubs; connection settings are in [README](../../README.md#hardware).

## Frame

```text
A5 5A | command:u8 | payload_length:u16 | payload:N octets | crc:u16
```

All integers are little-endian. Length counts payload octets only.
CRC-16/CCITT-FALSE covers command, length, and payload: polynomial `0x1021`,
initial `0xFFFF`, no reflection, xor-out zero; CRC is sent low octet first.
Maximum payload: 2,054 request octets, 2,049 response octets.
Start markers inside payloads are ordinary data.

## Commands

Addresses are physical NAND page/block numbers; offsets and lengths count octets.
Responses echo the command and begin with `status:u8`, followed by the data below.

| ID | Command | Request payload | Response data |
| --- | --- | --- | --- |
| `01` | `INFO` | Empty | Identity and geometry below |
| `02` | `READ_PAGE` | `page:u32, offset:u16, length:u16` | Requested data |
| `03` | `PROGRAM_PAGE` | `page:u32, offset:u16, data:N` | Empty |
| `04` | `ERASE_BLOCK` | `block:u16` | Empty |
| `05` | `IS_BAD_BLOCK` | `block:u16` | `bad:u8` (0 good, 1 unusable) |

`INFO` data is 16 octets, in this order:

```text
version:u8 (=1), manufacturer:u8, device_high:u8, device_low:u8
page_data_size:u16, page_spare_size:u16, pages_per_block:u16
block_count:u16, page_count:u32
```

## Status and behavior

- Driver statuses use the stable numeric values in
  [W25N01GVSFIGStatus_t](../../MSW/W25N01GVSFIG/W25N01GVSFIG.h).
  `00` means success; `01` means corrected ECC with valid read data.
- Protocol errors: `80` unknown command, `81` wrong payload length, `82` bad CRC.
  Errors return status only; `READ_PAGE` includes data only for `00` or `01`.
- Send one request and wait for its response. Bad CRC prevents execution.
- No frame timeout: interrupted or oversized frames require `APP_IF_reset()`
  in firmware or MCU reset. Oversized frames receive no response.
- Receive failure resets the parser; send failure aborts the response.
  A missing response does not prove that program/erase failed; do not blindly retry.
- Python owns file handling, bad-block allocation, verification, and the
  [programming restrictions](../../README.md#integration). Programming never erases first.
