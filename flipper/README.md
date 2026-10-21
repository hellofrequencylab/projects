# flipper

Flipper Zero external applications (`.fap`), built with
[ufbt](https://github.com/flipperdevices/flipperzero-ufbt).

    python3 -m pip install --upgrade ufbt
    ufbt update --channel=release
    cd apps/hello_freq && ufbt && ufbt launch

| App | What it does |
|---|---|
| [`apps/hello_freq`](apps/hello_freq) | Minimal ViewPort app — draws text, exits on Back. Starting point for new apps. |
| [`apps/frequency`](apps/frequency) | Tag writer for the Frequency CyberUnicorn console. Opens a second USB serial port and answers the console's `system.status`, `system.caps`, `hat.probe`, `frequency.write_tag` and `frequency.read_tag`, so the console writes check-in placards and member cards through the Flipper's NFC. |

Agent conventions and the SDK-grounding rules live in [`CLAUDE.md`](CLAUDE.md).

## Tools

- [`tools/placard_nfc.py`](tools/placard_nfc.py) builds a Flipper `.nfc` file holding one NDEF link, for the stock NFC app's **Write** on a blank NTAG213/215/216. No app needed.
