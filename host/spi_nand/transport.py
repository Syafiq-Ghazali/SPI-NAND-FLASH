import binascii
import math
import struct
import time

import serial


class CommunicationError(Exception):
    pass


class ProtocolError(CommunicationError):
    pass


class ResponseTimeout(CommunicationError):
    pass


class SerialTransport:
    """One request at a time. A failed exchange requires MCU reset and reopening."""

    def __init__(self, port: str, baudrate: int = 115200, timeout: float = 2.0):
        if not math.isfinite(timeout) or timeout <= 0:
            raise ValueError("timeout must be a positive, finite number")
        if isinstance(baudrate, bool) or not isinstance(baudrate, int) or baudrate <= 0:
            raise ValueError("baudrate must be a positive integer")
        self.timeout = timeout
        self._failed = False
        try:
            self._serial = serial.Serial(
                port=port, baudrate=baudrate, bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE,
                timeout=timeout, write_timeout=timeout,
                xonxoff=False, rtscts=False, dsrdtr=False,
            )
        except (serial.SerialException, OSError) as exc:
            raise CommunicationError(f"Cannot open {port}: {exc}") from exc

    def close(self) -> None:
        try:
            self._serial.close()
        except (serial.SerialException, OSError) as exc:
            raise CommunicationError(f"Cannot close serial port: {exc}") from exc

    def invalidate(self) -> None:
        self._failed = True

    def _remaining(self, deadline: float) -> float:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise ResponseTimeout("Serial exchange timed out")
        return remaining

    def _read_exact(self, count: int, deadline: float) -> bytes:
        data = bytearray()
        while len(data) < count:
            self._serial.timeout = self._remaining(deadline)
            chunk = self._serial.read(count - len(data))
            if not chunk:
                raise ResponseTimeout("Timed out waiting for a complete response")
            data.extend(chunk)
        return bytes(data)

    def exchange(self, command: int, payload: bytes = b"") -> bytes:
        if self._failed:
            raise CommunicationError("Connection is unsynchronized; reset the MCU and reopen it")
        if not self._serial.is_open:
            raise CommunicationError("Serial port is closed")
        if not 1 <= command <= 5:
            raise ValueError("command must be one of the five firmware commands")
        if not isinstance(payload, bytes):
            raise TypeError("payload must be bytes")
        if len(payload) > 2054:
            raise ValueError("request payload exceeds 2054 bytes")
        body = struct.pack("<BH", command, len(payload)) + payload
        frame = b"\xa5\x5a" + body + struct.pack("<H", binascii.crc_hqx(body, 0xFFFF))
        deadline = time.monotonic() + self.timeout
        try:
            sent = 0
            while sent < len(frame):
                self._serial.write_timeout = self._remaining(deadline)
                count = self._serial.write(frame[sent:])
                if not count:
                    raise ResponseTimeout("Timed out sending a complete request")
                sent += count

            previous = 0
            while True:
                octet = self._read_exact(1, deadline)[0]
                if previous == 0xA5 and octet == 0x5A:
                    break
                previous = octet

            header = self._read_exact(3, deadline)
            response_command, length = struct.unpack("<BH", header)
            if response_command != command:
                raise ProtocolError("Response command does not match the request")
            if not 1 <= length <= 2049:
                raise ProtocolError(f"Invalid response payload length: {length}")
            response = self._read_exact(length, deadline)
            checksum = struct.unpack("<H", self._read_exact(2, deadline))[0]
            if checksum != binascii.crc_hqx(header + response, 0xFFFF):
                raise ProtocolError("Response CRC does not match")
            return response
        except CommunicationError:
            self.invalidate()
            raise
        except KeyboardInterrupt as exc:
            self.invalidate()
            raise CommunicationError("Serial exchange interrupted") from exc
        except (serial.SerialException, OSError) as exc:
            self.invalidate()
            raise CommunicationError(f"Serial exchange failed: {exc}") from exc
