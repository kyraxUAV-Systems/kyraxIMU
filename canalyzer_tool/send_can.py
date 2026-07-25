#!/usr/bin/env python3
"""Interactively send one CAN data frame through a python-can adapter."""

import argparse
from typing import Optional

import can


def parse_can_id(value: str) -> int:
    can_id = int(value, 0)
    if not 0 <= can_id <= 0x1FFFFFFF:
        raise argparse.ArgumentTypeError("CAN ID must be between 0 and 0x1FFFFFFF")
    return can_id


def parse_data(value: str) -> bytes:
    tokens = value.replace(",", " ").split()
    if len(tokens) > 8:
        raise argparse.ArgumentTypeError("classic CAN data contains at most 8 bytes")
    try:
        data = bytes(int(token.removeprefix("0x"), 16) for token in tokens)
    except ValueError as error:
        raise argparse.ArgumentTypeError(
            "data must be hexadecimal bytes, for example: DE AD BE EF"
        ) from error
    if any(byte > 0xFF for byte in data):
        raise argparse.ArgumentTypeError("each data value must fit in one byte")
    return data


def prompt(prompt_text: str, default: Optional[str] = None) -> str:
    suffix = f" [{default}]" if default is not None else ""
    answer = input(f"{prompt_text}{suffix}: ").strip()
    return answer if answer else (default or "")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--channel", help="adapter channel/device path")
    parser.add_argument("--interface", default="slcan", help="python-can interface name")
    parser.add_argument("--bitrate", type=int, help="CAN bitrate in bit/s")
    parser.add_argument("--id", dest="can_id", type=parse_can_id, help="CAN ID, e.g. 0x321")
    parser.add_argument("--data", type=parse_data,
                        help="hex bytes, e.g. 'DE AD BE EF' (empty is allowed)")
    parser.add_argument("--extended", action="store_true", help="send a 29-bit identifier")
    args = parser.parse_args()

    channel = args.channel or prompt("Adapter channel", "/dev/cu.usbmodemAAAD204938841")
    bitrate = args.bitrate or int(prompt("Bitrate (bit/s)", "500000"), 0)
    can_id = args.can_id
    if can_id is None:
        can_id = parse_can_id(prompt("CAN ID", "0x321"))
    data = args.data
    if data is None:
        data = parse_data(prompt("Data bytes in hex (0 to 8, space separated)", "01 02 03 04"))

    id_limit = 0x1FFFFFFF if args.extended else 0x7FF
    if can_id > id_limit:
        parser.error("identifier does not fit the selected frame type")

    message = can.Message(arbitration_id=can_id, data=data,
                          is_extended_id=args.extended)
    with can.Bus(interface=args.interface, channel=channel, bitrate=bitrate) as bus:
        bus.send(message)

    payload = " ".join(f"{byte:02X}" for byte in data)
    print(f"Sent ID=0x{can_id:X} DLC={len(data)} DATA=[{payload}]")


if __name__ == "__main__":
    main()
