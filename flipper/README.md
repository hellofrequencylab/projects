# flipper

Flipper Zero external applications (`.fap`), built with
[ufbt](https://github.com/flipperdevices/flipperzero-ufbt).

    python3 -m pip install --upgrade ufbt
    ufbt update --channel=release
    cd apps/hello_freq && ufbt && ufbt launch

| App | What it does |
|---|---|
| [`apps/hello_freq`](apps/hello_freq) | Minimal ViewPort app — draws text, exits on Back. Starting point for new apps. |

Agent conventions and the SDK-grounding rules live in [`CLAUDE.md`](CLAUDE.md).
