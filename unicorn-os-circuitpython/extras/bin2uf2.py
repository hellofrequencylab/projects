#!/usr/bin/env python3
"""Convert an ESP32-S3 app .bin into a .uf2 for Adafruit's TinyUF2 bootloader.

Usage: python3 bin2uf2.py build/hello_tft.ino.bin hello_tft.uf2
Then double-tap Reset, and copy the .uf2 onto the FTHRS3BOOT drive.
"""
import struct
import sys

FAMILY_ESP32S3 = 0xC47E5767
PAYLOAD = 256


def convert(data: bytes, base: int = 0x0) -> bytes:
    blocks = [data[i:i + PAYLOAD] for i in range(0, len(data), PAYLOAD)]
    out = bytearray()
    for n, chunk in enumerate(blocks):
        header = struct.pack(
            "<IIIIIIII",
            0x0A324655, 0x9E5D5157,  # magic start
            0x00002000,              # flag: familyID present
            base + n * PAYLOAD,
            PAYLOAD,
            n,
            len(blocks),
            FAMILY_ESP32S3,
        )
        body = chunk.ljust(476, b"\x00")
        out += header + body + struct.pack("<I", 0x0AB16F30)
    return bytes(out)


if __name__ == "__main__":
    src, dst = sys.argv[1], sys.argv[2]
    with open(src, "rb") as f:
        uf2 = convert(f.read())
    with open(dst, "wb") as f:
        f.write(uf2)
    print(f"wrote {dst}: {len(uf2) // 512} blocks")
