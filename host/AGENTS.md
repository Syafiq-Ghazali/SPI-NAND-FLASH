# Python application guidance

Follow the repository's [AGENTS.md](../AGENTS.md).

## Current system

- Target: TMS320F280039C controlling a Winbond W25N01GVSFIG over SPI.
- Firmware has one application module, `ASW/APP_IF`, which decodes requests,
  dispatches directly to the NAND driver, and encodes responses. Do not introduce
  a separate flash service layer without a concrete need.
- `ASW/MAIN` initializes the board and NAND, then polls the SCI transport.
- The serial default is SCI-A, 115200 baud, 8N1, RX GPIO28 and TX GPIO29.
- Exactly five commands exist: INFO, READ_PAGE, PROGRAM_PAGE, ERASE_BLOCK,
  IS_BAD_BLOCK. Keep this command set minimal.
- The shared wire contract is [PROTOCOL.md](../ASW/APP_IF/PROTOCOL.md).
  Firmware implementation is the source of truth; update both sides and the
  protocol document together when changing it.
- This folder currently contains documentation only. The Python library and CLI
  are not implemented. Firmware compilation and mocked command tests do not
  establish that the physical serial/SPI connection works.

## Python implementation direction

- Use Python 3.10 or newer, a reusable client library, and a thin CLI.
- Keep serial transport/framing separate from flash operations. Start small;
  do not add a GUI, background workers, or a general storage framework.
- Use physical addresses, explicit little-endian fields, and octet lengths.
  A Python byte corresponds to one low-eight-bit `Uint16_t` element on the MCU.
- Serialize commands. Validate response command, length, CRC, and status.
- Treat ECC_CORRECTED as a successful read with a warning the caller can inspect.
  Surface all other driver and communication errors explicitly.
- Python owns file I/O, page splitting, bad-block allocation, programming order,
  partial-program limits, progress, and read-back verification.
- Do not automatically erase before programming, silently skip bad blocks in
  physical-address operations, or retry program/erase after a missing response.
  Report uncertain outcomes and allow inspection before proceeding.
- Provide scan and verify as combinations of the existing commands.
- Close the serial connection when finished and keep dependencies minimal.
- Exclude virtual environments, generated files, dumps, images, and local port
  configuration from Git.

## Validation

Run the firmware decoder tests from the repository root:

```text
python -m unittest discover -s tests -v
```

These compile the actual C application with mocked NAND operations and require
a native C compiler (GCC/Clang or Visual Studio C tools). Add client tests when
Python code is introduced. Test hardware separately; report unavailable checks.
