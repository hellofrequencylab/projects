#!/usr/bin/env python3
"""Build a Flipper Zero .nfc file that holds one NDEF URL, ready to Write to a blank NTAG.

Usage:
    python3 placard_nfc.py "<placard link>" --type NTAG215 -o Placard.nfc

The placard link is the Space's NFC link from frequency.so (Space settings > Check-in, or the
/admin/cyberunicorn console): `https://.../n/<node>?s=v1....&m=nfc`. The site signs it; this
script only packs it.

Format grounded in flipperzero-firmware 1.5.x:
  lib/nfc/nfc_device.c, lib/nfc/protocols/mf_ultralight/mf_ultralight.c (load + is_all_data_read),
  mf_ultralight_poller.c (write: pages 4 .. config_page-2, same type, unlocked target).
The NFC app only offers Write when every page is "read" and PWD/PACK are non-zero, so the
config pages carry the chip defaults (PWD FF FF FF FF).
"""
import argparse
import sys

# type: (total pages, config page, CC size byte, GET_VERSION storage byte)
TYPES = {
    "NTAG213": (45, 41, 0x12, 0x0F),
    "NTAG215": (135, 131, 0x3E, 0x11),
    "NTAG216": (231, 227, 0x6D, 0x13),
}

# NDEF URI prefix codes (NFC Forum URI RTD), longest first.
URI_PREFIXES = [(0x04, "https://"), (0x03, "http://"), (0x02, "https://www."), (0x01, "http://www.")]


def ndef_tlv(url: str) -> bytes:
    code, rest = 0x00, url
    for c, p in sorted(URI_PREFIXES, key=lambda x: -len(x[1])):
        if url.startswith(p):
            code, rest = c, url[len(p):]
            break
    payload = bytes([code]) + rest.encode("utf-8")
    if len(payload) > 255:
        sys.exit("URL too long for a short NDEF record")
    record = bytes([0xD1, 0x01, len(payload), 0x55]) + payload  # MB|ME|SR, TNF well-known, type "U"
    n = len(record)
    length = bytes([n]) if n < 0xFF else bytes([0xFF, n >> 8, n & 0xFF])
    return b"\x03" + length + record + b"\xFE"


def build(url: str, kind: str) -> str:
    total, cfg, cc_size, ver_size = TYPES[kind]
    user = ndef_tlv(url)
    capacity = (cfg - 1 - 4) * 4
    if len(user) > capacity:
        sys.exit(f"Link needs {len(user)} bytes; {kind} holds {capacity}")

    uid = [0x04, 0xF1, 0x1F, 0x5A, 0xC0, 0xDE, 0x01]  # placeholder; Write never touches UID pages
    bcc0 = 0x88 ^ uid[0] ^ uid[1] ^ uid[2]
    bcc1 = uid[3] ^ uid[4] ^ uid[5] ^ uid[6]

    mem = bytearray(total * 4)
    mem[0:12] = bytes(uid[0:3] + [bcc0] + uid[3:7] + [bcc1, 0x48, 0x00, 0x00])
    mem[12:16] = bytes([0xE1, 0x10, cc_size, 0x00])
    mem[16:16 + len(user)] = user
    mem[(cfg - 1) * 4:(cfg - 1) * 4 + 4] = bytes([0x00, 0x00, 0x00, 0xBD])  # dynamic lock, unlocked
    mem[cfg * 4:(cfg + 4) * 4] = bytes(
        [0x04, 0x00, 0x00, 0xFF,  # CFG0: AUTH0 = FF (no password protection)
         0x00, 0x05, 0x00, 0x00,  # CFG1
         0xFF, 0xFF, 0xFF, 0xFF,  # PWD (chip default)
         0x00, 0x00, 0x00, 0x00]  # PACK
    )

    hx = lambda b: " ".join(f"{x:02X}" for x in b)
    lines = [
        "Filetype: Flipper NFC device",
        "Version: 4",
        "Device type: NTAG/Ultralight",
        f"UID: {hx(uid)}",
        "ATQA: 00 44",
        "SAK: 00",
        "Data format version: 2",
        f"NTAG/Ultralight type: {kind}",
        f"Signature: {hx(bytes(32))}",
        f"Mifare version: {hx(bytes([0x00, 0x04, 0x04, 0x02, 0x01, 0x00, ver_size, 0x03]))}",
    ]
    for i in range(3):
        lines += [f"Counter {i}: 0", f"Tearing {i}: 00"]
    lines += [f"Pages total: {total}", f"Pages read: {total}"]
    lines += [f"Page {p}: {hx(mem[p * 4:p * 4 + 4])}" for p in range(total)]
    lines.append("Failed authentication attempts: 0")
    return "\n".join(lines) + "\n"


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("url")
    ap.add_argument("--type", default="NTAG215", choices=sorted(TYPES), help="must match the blank tag")
    ap.add_argument("-o", "--out", default="Placard.nfc")
    a = ap.parse_args()
    if "/n/" not in a.url or "s=v1." not in a.url:
        print("warning: this does not look like a signed placard link (/n/<id>?s=v1....)", file=sys.stderr)
    with open(a.out, "w") as f:
        f.write(build(a.url, a.type))
    print(f"wrote {a.out} ({a.type})")


if __name__ == "__main__":
    main()
