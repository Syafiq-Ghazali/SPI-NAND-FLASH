import ctypes
import importlib.util
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from spi_nand import FlashError, NandClient, Status

from test_client import FakeSerial


ROOT = Path(__file__).resolve().parents[2]


class FirmwareProtocolTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        spec = importlib.util.spec_from_file_location("firmware_fixture", ROOT / "tests/test_app_if.py")
        fixture = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(fixture)
        cls.directory = tempfile.TemporaryDirectory()
        try:
            cls.library = ctypes.CDLL(str(fixture.compile_fixture(Path(cls.directory.name))))
        except BaseException:
            cls.directory.cleanup()
            raise
        for name, arguments, result in (
            ("test_init", [], None),
            ("test_receive", [ctypes.c_uint16], None),
            ("test_status", [ctypes.c_uint16], None),
            ("test_output_length", [], ctypes.c_uint16),
            ("test_output", [ctypes.c_uint16], ctypes.c_uint16),
        ):
            function = getattr(cls.library, name)
            function.argtypes = arguments
            function.restype = result

    @classmethod
    def tearDownClass(cls):
        if os.name == "nt":
            import _ctypes
            _ctypes.FreeLibrary(cls.library._handle)
        del cls.library
        cls.directory.cleanup()

    def setUp(self):
        self.library.test_init()
        library = self.library

        class FirmwareSerial(FakeSerial):
            def write(self, data):
                before = library.test_output_length()
                for octet in data:
                    library.test_receive(octet)
                self.response.extend(library.test_output(i)
                                     for i in range(before, library.test_output_length()))
                return super().write(data)

        self.serial = FirmwareSerial(chunk_size=7)
        self.patcher = patch("spi_nand.transport.serial.Serial", return_value=self.serial)
        self.patcher.start()
        self.addCleanup(self.patcher.stop)
        self.client = NandClient("fixture")
        self.addCleanup(self.client.close)

    def test_all_five_commands_against_actual_c_decoder(self):
        self.assertEqual(self.client.info().page_count, 65536)
        self.assertEqual(self.client.read_page(65535).data, bytes(range(256)) * 8)
        self.client.program_page(123, b"\xa5\x5a" * 1024)
        self.client.erase_block(1023)
        self.assertTrue(self.client.is_bad_block(12))

    def test_corrected_ecc_and_driver_error_against_actual_c_decoder(self):
        self.library.test_status(1)
        self.assertTrue(self.client.read_page(0).ecc_corrected)
        self.library.test_status(15)
        with self.assertRaises(FlashError) as raised:
            self.client.read_page(0)
        self.assertEqual(raised.exception.status, Status.ECC_UNCORRECTABLE)


if __name__ == "__main__":
    unittest.main()
