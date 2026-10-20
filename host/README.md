# Python client

Python 3.10+ library and CLI for the five commands in the
[serial protocol](../ASW/APP_IF/PROTOCOL.md). The only runtime dependency is
pySerial. Firmware UART receive/send remain stubs, so real board communication
requires the UART driver first. Simulated tests do not establish hardware operation.

## Install

From the repository root:

```text
python -m venv host/.venv
host/.venv/Scripts/python -m pip install -e ./host
```

Activate the environment to use `spi-nand` directly. On Linux/macOS, use
`host/.venv/bin/python` and activate with `source host/.venv/bin/activate`.

## Library

```python
from spi_nand import NandClient

with NandClient("COM5") as flash:
    info = flash.info()
    print(info)
    print(flash.is_bad_block(10))
    result = flash.read_page(640)
    print(result.data, result.ecc_corrected)
```

`NandClient(port, baudrate=115200, timeout=2.0)` opens an 8N1 connection without
flow control. The timeout is a total budget for sending a request and receiving
its complete response. Call `close()` or use a `with` statement. Use one client
from one thread at a time, with one outstanding command.

| Method | Result |
| --- | --- |
| `info()` | `DeviceInfo` containing protocol version, ID, and geometry |
| `read_page(page, offset=0, length=2048)` | `ReadResult(data: bytes, ecc_corrected: bool)` |
| `program_page(page, data: bytes, offset=0)` | `None` on success |
| `erase_block(block)` | `None` on success |
| `is_bad_block(block)` | `bool` |

Addresses are physical: pages 0–65535, blocks 0–1023. Reads must fit within the
2048-byte data area. Program data and offsets must be multiples of 512 bytes
and fit within one page. Input is never padded. Programming does not erase or
verify automatically, and bad blocks are never silently skipped.

**Callers must program pages in ascending order within each block and limit each
page to four programs between erases.** The library cannot determine prior
programming history, including operations from previous sessions. Use an explicit
erase and a known programming sequence. Multi-page file transfers, bad-block
allocation, scan, and verify helpers are deferred.

## Errors and recovery

- Invalid arguments raise `ValueError` or `TypeError` before transmission.
- `FlashError` exposes `command` and `status` (`Status` enum). Uncorrectable ECC
  raises this error; corrected ECC returns valid data with `ecc_corrected=True`.
- `CommunicationError` covers serial failures; `ResponseTimeout` and
  `ProtocolError` distinguish missing/incomplete and malformed responses.
- `UncertainOutcomeError` means a program/erase exchange failed without a valid
  result. It exposes `command`. Inspect the flash before deciding to retry.

There are no automatic retries. After any exchange or response-validation failure,
the connection refuses further commands. Reset the MCU and reopen the client
before continuing; reopening the serial port alone does not reset the firmware
parser. Its current interrupted/oversized-frame behavior requires MCU reset.
A valid error response leaves the connection usable. A device timeout or failed
program/erase still requires inspection before further writes.

## CLI

With the environment activated:

```text
spi-nand --port COM5 info
spi-nand --port COM5 is-bad-block 10
spi-nand --port COM5 read-page 640 --output page.bin
spi-nand --port COM5 read-page 640 --offset 512 --length 512 --output sector.bin
spi-nand --port COM5 erase-block 10
spi-nand --port COM5 program-page 640 --input page.bin
```

These are individual examples; erase/program change flash contents. Keep the
programming restrictions above in mind when combining them. `--input` contains
exactly one page or aligned partial page (512, 1024, 1536, or 2048 bytes).
`--output` creates a new binary file and refuses to overwrite an existing file.
Corrected ECC warnings go to stderr. Commands exit with 0 on success and 1 on
operation errors; argument syntax errors exit with 2. Decimal and `0x` addresses
are supported. Use `--baudrate` or `--timeout` before the command to override defaults.

The CLI is also available through `python -m spi_nand.cli`.

## Development and validation

`transport.py` owns serial I/O, frames, CRC, and communication recovery.
`client.py` owns command payloads, argument validation, statuses, and results.
`cli.py` handles arguments and single-page files through the library.

Run from the repository root after installation:

```text
host/.venv/Scripts/python -m unittest discover -s host/tests -v
python -m unittest discover -s tests -v
```

Host tests cover simulated serial responses and exercise the actual C decoder
with mocked NAND operations. The latter tests need a native C compiler, as do
the existing firmware tests; they skip when one is unavailable. Board UART/SPI,
NAND operation timing, and erase/program/read-back remain hardware checks.
Keep local dumps and program images outside the repository.
