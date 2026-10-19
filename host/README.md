# Python application

This folder is reserved for the Python client library and CLI. They are not yet
implemented.

The MCU application includes INFO, READ_PAGE, PROGRAM_PAGE, ERASE_BLOCK, and
IS_BAD_BLOCK through one application module. UART is the intended connection;
its receive and send functions are stubs, so physical communication is not yet
implemented. See the shared
[serial protocol](../ASW/APP_IF/PROTOCOL.md) and [development guidance](AGENTS.md).
