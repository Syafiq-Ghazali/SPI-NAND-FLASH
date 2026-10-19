# SPI NAND serial protocol, version 1

The MCU application receives one request, calls the NAND driver, and sends one
response. There are exactly five commands. Addresses are physical NAND page and
block numbers. There is no filesystem, logical mapping, automatic erase, or
automatic retry.

## Connection and framing

Default: SCI-A, 115200 baud, 8 data bits, no parity, one stop bit, no flow control.
MCU RX is GPIO28; MCU TX is GPIO29. Connect adapter TX to MCU RX, adapter RX to
MCU TX, and a common ground using compatible 3.3 V logic. Check board routing
before connecting; pins and baud are configured in `BRD`.

Both directions use this frame:

```text
A5 5A | command:u8 | payload_length:u16 | payload:N octets | crc:u16
```

All multi-octet fields are little-endian. Length counts only payload octets.
CRC-16/CCITT-FALSE covers command, both length octets, and payload; it excludes
the start marker and CRC itself. Polynomial is `0x1021`, initial value `0xFFFF`,
no reflection, xor-out zero. The check value for ASCII `123456789` is `0x29B1`.
The CRC is transmitted low octet first.

Maximum request payload is 2,054 octets. Maximum response payload is 2,049.
Start-marker octets inside a payload have no special meaning. Buffers on C2000
use one `Uint16_t` element per wire octet, with only its low eight bits used.

## Commands

Response command echoes the request command. Every response payload starts with
one status octet. Unless stated otherwise, errors return only status.

| ID | Command | Request payload | Successful response after status |
| --- | --- | --- | --- |
| `01` | `INFO` | Empty | Geometry and identity below |
| `02` | `READ_PAGE` | `page:u32, offset:u16, length:u16` | Requested data |
| `03` | `PROGRAM_PAGE` | `page:u32, offset:u16, data:N` | Empty |
| `04` | `ERASE_BLOCK` | `block:u16` | Empty |
| `05` | `IS_BAD_BLOCK` | `block:u16` | `bad:u8`: 0 good, 1 unusable |

`INFO` returns 16 octets after status:

```text
protocol_version:u8 (=1)
manufacturer:u8, device_high:u8, device_low:u8
page_data_size:u16, page_spare_size:u16
pages_per_block:u16, block_count:u16, page_count:u32
```

Page numbers are 0..65535; block numbers are 0..1023. Reads must have a nonzero
length and remain inside the 2,048-octet page data area. Program data must be
nonempty, remain inside that area, and have 512-octet aligned offset and length.
Spare-area access is not exposed. Erase operates on a complete 128 KiB block.
Programming does not erase first.

## Status

Driver statuses are returned directly from `W25N01GVSFIGStatus_t`; their numeric
values are part of protocol version 1 and must remain stable.

| Value | Meaning | Value | Meaning |
| --- | --- | --- | --- |
| `00` | OK | `01` | ECC corrected; read data is valid |
| `02` | Null pointer | `03` | Not initialized |
| `04` | Invalid configuration | `05` | Invalid address |
| `06` | Invalid length | `07` | Invalid bit transition |
| `08` | SPI error | `09` | NAND ready timeout |
| `0A` | ID mismatch | `0B` | Write protected |
| `0C` | Bad block | `0D` | Program failed |
| `0E` | Erase failed | `0F` | ECC uncorrectable |
| `80` | Unknown command | `81` | Incorrect command payload length |
| `82` | CRC mismatch | | |

`READ_PAGE` includes data for statuses `00` and `01` only. Uncorrectable or
failed reads never transmit buffer contents as valid data.

## Timing and recovery

- Send one complete request and wait for its complete response. No pipelining.
- Wait at least one second after MCU reset before the first request. Initialization
  failure leaves the command loop running; driver commands report not initialized.
- A one-second gap between received octets discards an incomplete frame silently.
- Oversized frames are discarded without response until a one-second idle gap.
  This prevents a start marker in discarded data from being treated as a command.
- A complete frame with a bad CRC gets `82` and never executes its command.
- UART receive errors reset the receiver and discard the partial frame. A UART
  transmit timeout aborts the response. No operation is repeated automatically.
- After an incomplete response, stop transmission, wait for a one-second idle gap,
  then clear stale host input before starting another exchange. Do not assume a
  timed-out program or erase did not execute; inspect the affected flash before
  deciding what to do next. MCU reset also clears parser state.
- Host timeouts must allow full frame transmission and NAND execution; start with
  five seconds per exchange and validate against the hardware. Existing SPI
  driver blocking waits still need timeout hardening before fault recovery can
  be guaranteed for peripheral stalls.

Python owns file handling, page splitting, bad-block allocation, read-back
verification, ascending programming order within a block, and the limit of four
partial programs per page between erases. Rewriting data is not made safe merely
by passing the driver's zero-to-one transition check. Read-back verification uses
`READ_PAGE`, and scanning uses repeated `IS_BAD_BLOCK`; neither needs a new command.
