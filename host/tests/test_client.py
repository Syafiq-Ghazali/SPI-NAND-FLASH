import binascii
import contextlib
import io
import json
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

import serial

from spi_nand import (
    CommunicationError, FlashError, NandClient, ProtocolError, ResponseTimeout,
    Status, UncertainOutcomeError,
)
from spi_nand.cli import main
from spi_nand.transport import SerialTransport


def frame(command, payload):
    body = struct.pack("<BH", command, len(payload)) + payload
    return b"\xa5\x5a" + body + struct.pack("<H", binascii.crc_hqx(body, 0xFFFF))


GEOMETRY = struct.pack("<B3B4HI", 1, 0xEF, 0xAA, 0x21, 2048, 64, 64, 1024, 65536)


class FakeSerial:
    def __init__(self, response=b"", chunk_size=65536, write_size=65536):
        self.response = bytearray(response)
        self.written = bytearray()
        self.chunk_size = chunk_size
        self.write_size = write_size
        self.is_open = True
        self.timeout = 2
        self.write_timeout = 2
        self.write_error = None
        self.read_error = None

    def read(self, count):
        if self.read_error:
            raise self.read_error
        count = min(count, self.chunk_size)
        data = self.response[:count]
        del self.response[:count]
        return bytes(data)

    def write(self, data):
        if self.write_error:
            raise self.write_error
        data = data[:self.write_size]
        self.written.extend(data)
        return len(data)

    def close(self):
        self.is_open = False


class ClientTests(unittest.TestCase):
    def setUp(self):
        self.serial = FakeSerial()
        self.patcher = patch("spi_nand.transport.serial.Serial", return_value=self.serial)
        self.open_serial = self.patcher.start()
        self.addCleanup(self.patcher.stop)
        self.client = NandClient("COM5")
        self.addCleanup(self.client.close)

    def respond(self, command, payload):
        self.serial.response.extend(frame(command, payload))

    def test_info_and_serial_settings(self):
        self.respond(1, b"\x00" + GEOMETRY)
        with self.client as flash:
            info = flash.info()
        self.assertEqual((info.version, info.manufacturer, info.device_high, info.device_low),
                         (1, 0xEF, 0xAA, 0x21))
        self.assertEqual((info.page_data_size, info.page_spare_size, info.pages_per_block,
                          info.block_count, info.page_count), (2048, 64, 64, 1024, 65536))
        self.assertEqual(self.serial.written, frame(1, b""))
        self.assertFalse(self.serial.is_open)
        settings = self.open_serial.call_args.kwargs
        self.assertEqual((settings["baudrate"], settings["bytesize"], settings["parity"],
                          settings["stopbits"]), (115200, 8, "N", 1))
        self.assertFalse(settings["xonxoff"])
        self.assertFalse(settings["rtscts"])

    def test_full_page_read_preserves_binary_data_and_corrected_ecc(self):
        data = bytes(range(256)) * 8
        self.respond(2, b"\x01" + data)
        result = self.client.read_page(65535)
        self.assertEqual(result.data, data)
        self.assertTrue(result.ecc_corrected)
        self.assertEqual(self.serial.written, frame(2, struct.pack("<IHH", 65535, 0, 2048)))

    def test_partial_read(self):
        self.respond(2, b"\x00\xa5\x5a")
        result = self.client.read_page(0, 2046, 2)
        self.assertEqual(result.data, b"\xa5\x5a")
        self.assertFalse(result.ecc_corrected)

    def test_program_sends_exactly_one_request_and_does_not_erase(self):
        data = b"\xa5\x5a" * 1024
        self.respond(3, b"\x00")
        self.client.program_page(65535, data)
        self.assertEqual(self.serial.written, frame(3, struct.pack("<IH", 65535, 0) + data))

    def test_program_sector_at_last_offset(self):
        self.respond(3, b"\x00")
        self.client.program_page(0, b"\xff" * 512, 1536)
        self.assertEqual(self.serial.written, frame(3, struct.pack("<IH", 0, 1536) + b"\xff" * 512))

    def test_erase_and_bad_block(self):
        self.respond(4, b"\x00")
        self.respond(5, b"\x00\x01")
        self.respond(5, b"\x00\x00")
        self.client.erase_block(1023)
        self.assertTrue(self.client.is_bad_block(3))
        self.assertFalse(self.client.is_bad_block(0))
        self.assertEqual(self.serial.written,
                         frame(4, b"\xff\x03") + frame(5, b"\x03\x00") + frame(5, b"\x00\x00"))

    def test_flash_errors_preserve_status_and_connection(self):
        for status in Status:
            if status in (Status.OK, Status.ECC_CORRECTED):
                continue
            with self.subTest(status=status):
                self.respond(2, bytes([status]))
                with self.assertRaises(FlashError) as raised:
                    self.client.read_page(0)
                self.assertEqual(raised.exception.status, status)
                self.assertEqual(raised.exception.command, 2)
        self.respond(1, b"\x00" + GEOMETRY)
        self.assertEqual(self.client.info().page_count, 65536)

    def test_program_and_erase_driver_failures_are_not_communication_errors(self):
        for command, operation, status in (
            (3, lambda: self.client.program_page(0, b"\xff" * 512), Status.PROGRAM_FAILED),
            (4, lambda: self.client.erase_block(0), Status.ERASE_FAILED),
        ):
            self.respond(command, bytes([status]))
            with self.assertRaises(FlashError) as raised:
                operation()
            self.assertEqual(raised.exception.status, status)

    def test_invalid_arguments_never_send(self):
        operations = [
            lambda: self.client.read_page(-1), lambda: self.client.read_page(65536),
            lambda: self.client.read_page(True), lambda: self.client.read_page(1.0),
            lambda: self.client.read_page(0, -1), lambda: self.client.read_page(0, 2048),
            lambda: self.client.read_page(0, 0, 0), lambda: self.client.read_page(0, 1, 2048),
            lambda: self.client.program_page(0, b""), lambda: self.client.program_page(0, b"x"),
            lambda: self.client.program_page(0, b"x" * 2560),
            lambda: self.client.program_page(0, b"x" * 512, 1),
            lambda: self.client.program_page(0, b"x" * 1024, 1536),
            lambda: self.client.program_page(0, "x" * 512),
            lambda: self.client.erase_block(-1), lambda: self.client.erase_block(1024),
            lambda: self.client.is_bad_block(1024),
        ]
        for operation in operations:
            with self.subTest(operation=operation), self.assertRaises((ValueError, TypeError)):
                operation()
        self.assertEqual(self.serial.written, b"")

    def test_chunked_reads_writes_and_noise(self):
        self.serial.chunk_size = 1
        self.serial.write_size = 2
        self.serial.response.extend(b"noise\xa5\xa5")
        self.respond(1, b"\x00" + GEOMETRY)
        self.assertEqual(self.client.info().page_count, 65536)
        self.assertEqual(self.serial.written, frame(1, b""))

    def test_response_crc_failure_blocks_further_requests(self):
        response = bytearray(frame(1, b"\x00" + GEOMETRY))
        response[-1] ^= 1
        self.serial.response.extend(response)
        with self.assertRaises(ProtocolError):
            self.client.info()
        self.respond(1, b"\x00" + GEOMETRY)
        with self.assertRaisesRegex(CommunicationError, "reset the MCU"):
            self.client.info()
        self.assertEqual(self.serial.written, frame(1, b""))

    def test_timeout_does_not_retry(self):
        self.serial.response.extend(frame(1, b"\x00" + GEOMETRY)[:-1])
        with self.assertRaises(ResponseTimeout):
            self.client.info()
        self.assertEqual(self.serial.written, frame(1, b""))

    def test_missing_mutation_response_is_uncertain(self):
        for command, operation, payload in (
            (3, lambda: self.client.program_page(0, b"\xff" * 512), b"\x00" * 6 + b"\xff" * 512),
            (4, lambda: self.client.erase_block(0), b"\x00\x00"),
        ):
            with self.subTest(command=command):
                self.client = NandClient("COM5")
                self.serial.written.clear()
                with self.assertRaises(UncertainOutcomeError) as raised:
                    operation()
                self.assertEqual(raised.exception.command, command)
                self.assertEqual(self.serial.written, frame(command, payload))

    def test_bad_mutation_response_is_uncertain(self):
        self.respond(4, b"\x00extra")
        with self.assertRaises(UncertainOutcomeError):
            self.client.erase_block(0)

    def test_serial_errors_are_wrapped(self):
        self.serial.read_error = serial.SerialException("disconnected")
        with self.assertRaisesRegex(CommunicationError, "disconnected"):
            self.client.info()

    def test_partial_write_failure_is_uncertain(self):
        self.serial.write_error = serial.SerialTimeoutException("partial write")
        with self.assertRaises(UncertainOutcomeError):
            self.client.erase_block(0)

    def test_interrupted_erase_is_uncertain_and_blocks_further_commands(self):
        self.serial.read_error = KeyboardInterrupt()
        with self.assertRaisesRegex(UncertainOutcomeError, "interrupted"):
            self.client.erase_block(0)
        self.serial.read_error = None
        with self.assertRaisesRegex(CommunicationError, "reset the MCU"):
            self.client.info()
        self.assertEqual(self.serial.written, frame(4, b"\x00\x00"))

    def test_no_write_progress_is_bounded(self):
        self.serial.write_size = 0
        with self.assertRaises(ResponseTimeout):
            self.client.info()

    def test_exchange_uses_one_total_deadline(self):
        self.respond(1, b"\x00" + GEOMETRY)
        with patch("spi_nand.transport.time.monotonic", side_effect=[0, 0, 1, 3]):
            with self.assertRaises(ResponseTimeout):
                self.client.info()

    def test_malformed_response_headers(self):
        for command, length in ((2, 1), (1, 0), (1, 2050), (1, 65535)):
            with self.subTest(command=command, length=length):
                self.client = NandClient("COM5")
                self.serial.response = bytearray(b"\xa5\x5a" + struct.pack("<BH", command, length))
                with self.assertRaises(ProtocolError):
                    self.client.info()

    def test_malformed_response_bodies(self):
        for command, payload, operation in (
            (1, b"\xff", lambda: self.client.info()),
            (1, b"\x00", lambda: self.client.info()),
            (1, b"\x03extra", lambda: self.client.info()),
            (1, b"\x01", lambda: self.client.info()),
            (1, b"\x00\x02" + GEOMETRY[1:], lambda: self.client.info()),
            (1, b"\x00" + GEOMETRY[:-1] + b"\x02", lambda: self.client.info()),
            (2, b"\x00short", lambda: self.client.read_page(0)),
            (5, b"\x00\x02", lambda: self.client.is_bad_block(0)),
        ):
            with self.subTest(command=command, payload=payload):
                self.client = NandClient("COM5")
                self.serial.response.clear()
                self.respond(command, payload)
                with self.assertRaises(ProtocolError):
                    operation()
                self.assertTrue(self.client._transport._failed)

    def test_closed_port(self):
        self.client.close()
        with self.assertRaisesRegex(CommunicationError, "closed"):
            self.client.info()

    def test_transport_rejects_invalid_settings(self):
        for timeout in (0, -1, float("inf"), float("nan")):
            with self.subTest(timeout=timeout), self.assertRaises(ValueError):
                SerialTransport("COM5", timeout=timeout)
        for baudrate in (0, -1, True, 1.5):
            with self.subTest(baudrate=baudrate), self.assertRaises(ValueError):
                SerialTransport("COM5", baudrate=baudrate)

    def test_open_failure_is_clear(self):
        self.open_serial.side_effect = serial.SerialException("port missing")
        with self.assertRaisesRegex(CommunicationError, "Cannot open COM9"):
            NandClient("COM9")


class CliTests(unittest.TestCase):
    def setUp(self):
        self.serial = FakeSerial()
        self.patcher = patch("spi_nand.transport.serial.Serial", return_value=self.serial)
        self.open_serial = self.patcher.start()
        self.addCleanup(self.patcher.stop)
        self.stdout = io.StringIO()
        self.stderr = io.StringIO()

    def run_cli(self, *args):
        with contextlib.redirect_stdout(self.stdout), contextlib.redirect_stderr(self.stderr):
            return main(["--port", "COM5", *args])

    def test_info(self):
        self.serial.response.extend(frame(1, b"\x00" + GEOMETRY))
        self.assertEqual(self.run_cli("info"), 0)
        self.assertEqual(json.loads(self.stdout.getvalue())["page_count"], 65536)
        self.assertFalse(self.serial.is_open)

    def test_read_binary_and_ecc_warning_without_overwriting_files(self):
        with tempfile.TemporaryDirectory() as tmp:
            output = Path(tmp) / "page.bin"
            self.serial.response.extend(frame(2, b"\x01\x00\xff\xa5\x5a"))
            self.assertEqual(self.run_cli("read-page", "0x10", "--length", "4", "--output", str(output)), 0)
            self.assertEqual(output.read_bytes(), b"\x00\xff\xa5\x5a")
            self.assertIn("ECC", self.stderr.getvalue())
            self.serial.response.extend(frame(2, b"\x00xxxx"))
            self.assertEqual(self.run_cli("read-page", "16", "--length", "4", "--output", str(output)), 1)
            self.assertEqual(output.read_bytes(), b"\x00\xff\xa5\x5a")

    def test_program_one_page_from_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / "page.bin"
            source.write_bytes(b"\xff" * 512)
            self.serial.response.extend(frame(3, b"\x00"))
            self.assertEqual(self.run_cli("program-page", "2", "--offset", "512", "--input", str(source)), 0)
            self.assertEqual(self.serial.written, frame(3, struct.pack("<IH", 2, 512) + b"\xff" * 512))

    def test_invalid_file_size_does_not_open_serial(self):
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / "page.bin"
            for data in (b"", b"x", b"x" * 2049):
                source.write_bytes(data)
                self.assertEqual(self.run_cli("program-page", "0", "--input", str(source)), 1)
        self.open_serial.assert_not_called()

    def test_erase_and_bad_block(self):
        self.serial.response.extend(frame(4, b"\x00"))
        self.assertEqual(self.run_cli("erase-block", "4"), 0)
        self.serial.is_open = True
        self.serial.response.extend(frame(5, b"\x00\x01"))
        self.assertEqual(self.run_cli("is-bad-block", "4"), 0)
        self.assertIn("bad", self.stdout.getvalue())

    def test_failed_mutation_reports_uncertainty_and_closes(self):
        self.assertEqual(self.run_cli("erase-block", "0"), 1)
        self.assertIn("outcome is uncertain", self.stderr.getvalue())
        self.assertFalse(self.serial.is_open)


if __name__ == "__main__":
    unittest.main()
