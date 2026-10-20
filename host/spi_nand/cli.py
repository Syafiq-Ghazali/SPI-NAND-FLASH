import argparse
from dataclasses import asdict
import json
from pathlib import Path
import sys

from . import CommunicationError, FlashError, NandClient


def _number(value: str) -> int:
    try:
        return int(value, 0)
    except ValueError as exc:
        raise argparse.ArgumentTypeError("expected an integer (decimal or 0x-prefixed)") from exc


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Physical page/block access to SPI NAND firmware")
    parser.add_argument("--port", required=True, help="serial port, e.g. COM5 or /dev/ttyUSB0")
    parser.add_argument("--baudrate", type=int, default=115200)
    parser.add_argument("--timeout", type=float, default=2.0, help="exchange timeout in seconds (default: 2)")
    commands = parser.add_subparsers(dest="command", required=True)
    commands.add_parser("info")
    read = commands.add_parser("read-page")
    read.add_argument("page", type=_number)
    read.add_argument("--offset", type=_number, default=0)
    read.add_argument("--length", type=_number, default=2048)
    read.add_argument("--output", type=Path, required=True, help="create a new binary file")
    program = commands.add_parser("program-page", help="program one page without erasing")
    program.add_argument("page", type=_number)
    program.add_argument("--offset", type=_number, default=0)
    program.add_argument("--input", type=Path, required=True, help="512, 1024, 1536, or 2048 bytes")
    erase = commands.add_parser("erase-block", help="erase one physical block")
    erase.add_argument("block", type=_number)
    bad = commands.add_parser("is-bad-block")
    bad.add_argument("block", type=_number)
    args = parser.parse_args(argv)

    try:
        data = b""
        if args.command == "program-page":
            with args.input.open("rb") as source:
                data = source.read(2049)
            if not data or len(data) > 2048 or len(data) % 512:
                raise ValueError("input must contain 512, 1024, 1536, or 2048 bytes")
        with NandClient(args.port, args.baudrate, args.timeout) as flash:
            if args.command == "info":
                print(json.dumps(asdict(flash.info()), indent=2))
            elif args.command == "read-page":
                result = flash.read_page(args.page, args.offset, args.length)
                with args.output.open("xb") as destination:
                    destination.write(result.data)
                if result.ecc_corrected:
                    print("Warning: read data required ECC correction", file=sys.stderr)
            elif args.command == "program-page":
                flash.program_page(args.page, data, args.offset)
                print("Page programmed")
            elif args.command == "erase-block":
                flash.erase_block(args.block)
                print("Block erased")
            elif args.command == "is-bad-block":
                print("bad" if flash.is_bad_block(args.block) else "good")
    except (CommunicationError, FlashError, OSError, ValueError) as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
