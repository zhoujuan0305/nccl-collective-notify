#!/usr/bin/env python3
import argparse
import datetime as dt
import socket
import struct


MSG_STRUCT = struct.Struct("<QQ")


def format_timestamp_us(timestamp_us: int) -> str:
    try:
        ts = dt.datetime.fromtimestamp(timestamp_us / 1_000_000, tz=dt.timezone.utc)
        return ts.isoformat()
    except (OverflowError, OSError, ValueError):
        return "invalid"


def main() -> None:
    parser = argparse.ArgumentParser(description="Receive forwarded NCCL collective notify messages.")
    parser.add_argument("--host", default="0.0.0.0", help="UDP bind host, default: 0.0.0.0")
    parser.add_argument("--port", type=int, default=19090, help="UDP bind port, default: 19090")
    args = parser.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((args.host, args.port))

    print(f"listening on udp://{args.host}:{args.port}")
    print("payload format: struct NcclCollectiveNotify { uint64_t timestamp_us; uint64_t collective_id; }")

    while True:
        data, addr = sock.recvfrom(4096)
        print("-" * 72)
        print(f"from: {addr[0]}:{addr[1]}")
        print(f"payload_len: {len(data)}")
        print(f"payload_hex: {data.hex()}")
        if len(data) == MSG_STRUCT.size:
            timestamp_us, collective_id = MSG_STRUCT.unpack(data)
            print(f"timestamp_us: {timestamp_us}")
            print(f"timestamp_iso_utc: {format_timestamp_us(timestamp_us)}")
            print(f"collective_id: {collective_id}")
        else:
            print("payload_parse: unexpected payload size")


if __name__ == "__main__":
    main()
