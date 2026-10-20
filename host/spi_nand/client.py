from dataclasses import dataclass
from enum import IntEnum
import struct

from .transport import CommunicationError, ProtocolError, SerialTransport


class Status(IntEnum):
    OK = 0
    ECC_CORRECTED = 1
    NULL_PTR = 2
    NOT_INITIALIZED = 3
    INVALID_CONFIG = 4
    INVALID_ADDRESS = 5
    INVALID_LENGTH = 6
    INVALID_BIT_TRANSITION = 7
    SPI_ERROR = 8
    TIMEOUT = 9
    ID_MISMATCH = 10
    WRITE_PROTECTED = 11
    BAD_BLOCK = 12
    PROGRAM_FAILED = 13
    ERASE_FAILED = 14
    ECC_UNCORRECTABLE = 15
    UNKNOWN_COMMAND = 0x80
    BAD_LENGTH = 0x81
    BAD_CRC = 0x82


class FlashError(Exception):
    def __init__(self, command: int, status: Status):
        self.command = command
        self.status = status
        super().__init__(f"Command {command}: {status.name} (0x{status:02x})")


class UncertainOutcomeError(CommunicationError):
    def __init__(self, command: int, reason: Exception):
        self.command = command
        operation = "Program" if command == 3 else "Erase"
        super().__init__(f"{operation} outcome is uncertain: {reason}. Inspect the flash before retrying")


@dataclass(frozen=True)
class DeviceInfo:
    version: int
    manufacturer: int
    device_high: int
    device_low: int
    page_data_size: int
    page_spare_size: int
    pages_per_block: int
    block_count: int
    page_count: int


@dataclass(frozen=True)
class ReadResult:
    data: bytes
    ecc_corrected: bool


def _integer(name: str, value: int, minimum: int, maximum: int) -> None:
    if isinstance(value, bool) or not isinstance(value, int):
        raise TypeError(f"{name} must be an integer")
    if not minimum <= value <= maximum:
        raise ValueError(f"{name} must be between {minimum} and {maximum}")


class NandClient:
    """Synchronous physical-address API; use from one thread at a time."""

    def __init__(self, port: str, baudrate: int = 115200, timeout: float = 2.0):
        self._transport = SerialTransport(port, baudrate, timeout)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.close()

    def close(self) -> None:
        self._transport.close()

    def _request(self, command: int, payload: bytes, data_length: int) -> tuple[Status, bytes]:
        try:
            response = self._transport.exchange(command, payload)
            try:
                status = Status(response[0])
            except (IndexError, ValueError) as exc:
                raise ProtocolError("Response has a missing or unknown status") from exc
            success = status == Status.OK or (command == 2 and status == Status.ECC_CORRECTED)
            expected = data_length + 1 if success else 1
            if len(response) != expected:
                raise ProtocolError(f"Command {command} expected {expected} response bytes, got {len(response)}")
            if status == Status.ECC_CORRECTED and command != 2:
                raise ProtocolError("Corrected ECC status is only valid for page reads")
            if not success:
                raise FlashError(command, status)
            return status, response[1:]
        except CommunicationError as exc:
            self._transport.invalidate()
            if command in (3, 4):
                raise UncertainOutcomeError(command, exc) from exc
            raise

    def info(self) -> DeviceInfo:
        _, data = self._request(1, b"", 16)
        info = DeviceInfo(*struct.unpack("<B3B4HI", data))
        if info.version != 1 or (
            info.page_data_size, info.page_spare_size, info.pages_per_block,
            info.block_count, info.page_count,
        ) != (2048, 64, 64, 1024, 65536):
            self._transport.invalidate()
            raise ProtocolError("Unsupported firmware version or flash geometry")
        return info

    def read_page(self, page: int, offset: int = 0, length: int = 2048) -> ReadResult:
        _integer("page", page, 0, 65535)
        _integer("offset", offset, 0, 2047)
        _integer("length", length, 1, 2048 - offset)
        status, data = self._request(2, struct.pack("<IHH", page, offset, length), length)
        return ReadResult(data, status == Status.ECC_CORRECTED)

    def program_page(self, page: int, data: bytes, offset: int = 0) -> None:
        """Caller enforces ascending pages and at most four programs per page per erase."""
        _integer("page", page, 0, 65535)
        _integer("offset", offset, 0, 1536)
        if not isinstance(data, bytes):
            raise TypeError("data must be bytes")
        if not data or offset + len(data) > 2048 or offset % 512 or len(data) % 512:
            raise ValueError("offset and data length must be multiples of 512 and fit within a page")
        self._request(3, struct.pack("<IH", page, offset) + data, 0)

    def erase_block(self, block: int) -> None:
        _integer("block", block, 0, 1023)
        self._request(4, struct.pack("<H", block), 0)

    def is_bad_block(self, block: int) -> bool:
        _integer("block", block, 0, 1023)
        _, data = self._request(5, struct.pack("<H", block), 1)
        if data[0] not in (0, 1):
            self._transport.invalidate()
            raise ProtocolError("Bad-block result must be 0 or 1")
        return bool(data[0])
