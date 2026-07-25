#!/usr/bin/env python3
"""Print CAN frames from a USB-CAN adapter using python-can."""

import argparse

import can


def parse_can_id(value: str) -> int:
    can_id = int(value, 0)
    if not 0 <= can_id <= 0x1FFFFFFF:
        raise argparse.ArgumentTypeError("CAN ID must be between 0 and 0x1FFFFFFF")
    return can_id


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--channel", default="/dev/cu.usbmodemAAAD204938841",
                        help="adapter channel/device path")
    parser.add_argument("--interface", default="slcan", help="python-can interface name")
    parser.add_argument("--bitrate", default=500000, type=int, help="CAN bitrate in bit/s")
    parser.add_argument("--id", dest="can_id", type=parse_can_id,
                        help="optional CAN identifier to display, for example 0x321")
    parser.add_argument("--extended", action="store_true",
                        help="match extended frames when --id is supplied")
    args = parser.parse_args()

    filters = None
    if args.can_id is not None:
        filters = [{
            "can_id": args.can_id,
            "can_mask": 0x1FFFFFFF if args.extended else 0x7FF,
            "extended": args.extended,
        }]

    with can.Bus(interface=args.interface, channel=args.channel,
                 bitrate=args.bitrate, can_filters=filters) as bus:
        print(f"Listening on {args.channel} at {args.bitrate} bit/s. Press Ctrl-C to stop.")
        for message in bus:
            payload = " ".join(f"{byte:02X}" for byte in message.data)
            frame_type = "extended" if message.is_extended_id else "standard"
            print(f"{message.timestamp:.6f}  ID=0x{message.arbitration_id:X} "
                  f"{frame_type} DLC={message.dlc} DATA=[{payload}]")


if __name__ == "__main__":
    main()
