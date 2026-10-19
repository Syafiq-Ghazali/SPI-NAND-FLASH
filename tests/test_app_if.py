import binascii
import ctypes
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


def frame(command, payload=b""):
    body = struct.pack("<BH", command, len(payload)) + payload
    return b"\xa5\x5a" + body + struct.pack("<H", binascii.crc_hqx(body, 0xFFFF))


def compile_fixture(directory):
    sources = [ROOT / "ASW/APP_IF/APP_IF.c", ROOT / "tests/app_if_fixture.c"]
    includes = [ROOT / "tests/stubs", ROOT / "LIB", ROOT / "ASW/APP_IF",
                ROOT / "MSW/W25N01GVSFIG"]
    library = directory / ("app_if.dll" if os.name == "nt" else "app_if.so")
    compiler = os.environ.get("CC") or shutil.which("cc") or shutil.which("gcc")
    if compiler:
        command = [compiler, "-std=c99", "-Wall", "-Wextra", "-Werror", "-shared"]
        if os.name != "nt":
            command.append("-fPIC")
        command += [f"-I{path}" for path in includes]
        command += [str(path) for path in sources] + ["-o", str(library)]
    elif os.name == "nt":
        installations = Path(os.environ.get("ProgramFiles", "C:/Program Files"))
        candidates = list(installations.glob(
            "Microsoft Visual Studio/*/*/VC/Auxiliary/Build/vcvars64.bat"))
        if not candidates:
            raise unittest.SkipTest("A native C compiler is required")
        arguments = ["cl", "/nologo", "/LD", "/W4", "/WX", "/std:c11"]
        arguments += [f"/I{path}" for path in includes]
        arguments += [str(path) for path in sources]
        arguments += ["/link", f"/OUT:{library}"]
        script = directory / "build_fixture.cmd"
        script.write_text(
            f'@echo off\ncall "{sorted(candidates)[-1]}" >nul\n'
            'if errorlevel 1 exit /b 1\n'
            + subprocess.list2cmdline(arguments) + '\n', encoding="utf-8")
        command = ["cmd.exe", "/d", "/c", str(script)]
    else:
        raise unittest.SkipTest("A native C compiler is required")
    result = subprocess.run(command, cwd=directory, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    return library


class AppIfTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.directory = tempfile.TemporaryDirectory()
        try:
            cls.library = ctypes.CDLL(str(compile_fixture(Path(cls.directory.name))))
        except BaseException:
            cls.directory.cleanup()
            raise
        signatures = {
            "test_init": ([], None),
            "test_receive": ([ctypes.c_uint16], None),
            "test_reset": ([], None),
            "test_status": ([ctypes.c_uint16], None),
            "test_send_limit": ([ctypes.c_uint16], None),
            "test_calls": ([], ctypes.c_uint16),
            "test_output_length": ([], ctypes.c_uint16),
            "test_output": ([ctypes.c_uint16], ctypes.c_uint16),
            "test_argument": ([ctypes.c_uint16], ctypes.c_uint32),
        }
        for name, (args, result) in signatures.items():
            function = getattr(cls.library, name)
            function.argtypes = args
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

    def send(self, data):
        for octet in data:
            self.library.test_receive(octet)

    def response(self, command, payload):
        actual = bytes(self.library.test_output(index)
                       for index in range(self.library.test_output_length()))
        self.assertEqual(actual, frame(command, payload))

    def test_info_identity_geometry_and_crc(self):
        self.send(frame(1))
        geometry = struct.pack("<B3B4HI", 1, 0xEF, 0xAA, 0x21,
                               2048, 64, 64, 1024, 65536)
        self.response(1, b"\x00" + geometry)

    def test_full_page_read_and_corrected_ecc(self):
        self.library.test_status(1)
        self.send(frame(2, struct.pack("<IHH", 65535, 0, 2048)))
        self.response(2, b"\x01" + bytes(range(256)) * 8)
        self.assertEqual(self.library.test_argument(0), 65535)

    def test_read_errors_exclude_data(self):
        for status in (3, 8, 9, 15):
            with self.subTest(status=status):
                self.library.test_init()
                self.library.test_status(status)
                self.send(frame(2, struct.pack("<IHH", 123, 512, 512)))
                self.response(2, bytes([status]))

    def test_program_maximum_payload_and_embedded_start(self):
        data = b"\xa5\x5a" * 1024
        self.send(frame(3, struct.pack("<IH", 0x1234, 0) + data))
        self.response(3, b"\x00")
        self.assertEqual([self.library.test_argument(i) for i in range(4)],
                         [0x1234, 0, 2048, sum(data)])
        self.assertEqual(self.library.test_calls(), 1)

    def test_erase_and_bad_block(self):
        self.send(frame(4, struct.pack("<H", 1023)))
        self.response(4, b"\x00")
        self.assertEqual(self.library.test_argument(0), 1023)
        self.library.test_init()
        self.send(frame(5, struct.pack("<H", 12)))
        self.response(5, b"\x00\x01")

    def test_driver_failure_is_preserved(self):
        for command, status in ((3, 13), (4, 14), (5, 8), (1, 3)):
            with self.subTest(command=command):
                self.library.test_init()
                self.library.test_status(status)
                payload = {1: b"", 3: struct.pack("<IH", 0, 0) + b"\xff" * 512,
                           4: b"\x00\x00", 5: b"\x00\x00"}[command]
                self.send(frame(command, payload))
                self.response(command, bytes([status]))

    def test_incorrect_payload_shapes_never_call_driver(self):
        for command, payload in ((1, b"x"), (2, b""), (3, b"\x00" * 6),
                                 (4, b"\x00"), (5, b"\x00" * 3)):
            with self.subTest(command=command):
                self.library.test_init()
                self.send(frame(command, payload))
                self.response(command, b"\x81")
                self.assertEqual(self.library.test_calls(), 0)

    def test_invalid_read_lengths_do_not_overrun_response(self):
        for length in (0, 2049, 65535):
            self.library.test_init()
            self.send(frame(2, struct.pack("<IHH", 0, 0, length)))
            self.response(2, b"\x06")
            self.assertEqual(self.library.test_calls(), 0)

    def test_unknown_command(self):
        self.send(frame(0xFF))
        self.response(0xFF, b"\x80")
        self.assertEqual(self.library.test_calls(), 0)

    def test_corrupt_crc_prevents_erase_and_receiver_recovers(self):
        damaged = bytearray(frame(4, b"\x0c\x00"))
        damaged[-1] ^= 1
        self.send(damaged)
        self.assertEqual(self.library.test_calls(), 0)
        first = frame(4, b"\x82")
        self.send(frame(4, b"\x0c\x00"))
        actual = bytes(self.library.test_output(i)
                       for i in range(self.library.test_output_length()))
        self.assertEqual(actual, first + frame(4, b"\x00"))
        self.assertEqual(self.library.test_calls(), 1)

    def test_incomplete_frame_is_discarded_by_reset(self):
        self.send(frame(3, struct.pack("<IH", 0, 0) + b"x" * 512)[:20])
        self.assertEqual(self.library.test_calls(), 0)
        self.library.test_reset()
        self.send(frame(1))
        self.assertEqual(self.library.test_calls(), 1)
        self.assertEqual(self.library.test_output(2), 1)

    def test_oversize_discards_embedded_commands_until_reset(self):
        self.send(b"\xa5\x5a\x03\xff\xff" + frame(4, b"\x00\x00"))
        self.assertEqual(self.library.test_calls(), 0)
        self.assertEqual(self.library.test_output_length(), 0)
        self.library.test_reset()
        self.send(frame(4, b"\x0c\x00"))
        self.response(4, b"\x00")
        self.assertEqual(self.library.test_calls(), 1)

    def test_noise_and_overlapping_start_markers(self):
        self.send(b"noise\xa5\xa5" + frame(4, b"\x0c\x00"))
        self.response(4, b"\x00")

    def test_failed_response_send_does_not_repeat_operation(self):
        self.library.test_send_limit(4)
        self.send(frame(4, b"\x0c\x00"))
        self.assertEqual(self.library.test_output_length(), 4)
        self.assertEqual(self.library.test_calls(), 1)


if __name__ == "__main__":
    unittest.main()
