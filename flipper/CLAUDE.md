# Flipper Zero — the SDK is the source of truth

**This is not the Flipper API you remember.** The firmware API changes between
releases, external apps are hard-gated by an API version, and this API is thin
in training data. Recalled function signatures are the primary failure mode in
this directory. Ground every call before writing it.

## Pinned toolchain

- SDK channel: **`release`** (matches stock firmware). Changing this is a
  deliberate change to this file, not a flag you pass once.
- Target: **f7** (Flipper Zero hardware).
- Install: `python3 -m pip install --upgrade ufbt && ufbt update --channel=release`
- SDK unpacks to `~/.ufbt` (override with `UFBT_HOME`).

The SDK channel must match the firmware on the device. A `.fap` built against
a different API version refuses to load with an API mismatch error — that is
the error, not a corrupt file.

## Before writing or changing any firmware call

1. **Check the API allowlist.** Symbols not in it do not link. This is the
   fastest possible check on an invented API:

       find ~/.ufbt -name api_symbols.csv        # locate it for your SDK version
       grep -i '<symbol>' "$(find ~/.ufbt -name api_symbols.csv | head -1)"

2. **Read the actual header** for any type or function before use:

       grep -rn 'view_dispatcher_alloc' ~/.ufbt/current/sdk/

3. **Prefer adapting a working example** over generating GUI boilerplate.
   `ViewDispatcher` / `SceneManager` scaffolding is where generated code goes
   wrong most often. Sources, in order of preference:
   - `ufbt create APPID=x` in a scratch dir — emits the canonical template for
     *your* SDK version, which beats any template committed here.
   - `applications/examples/` in flipperdevices/flipperzero-firmware.

Never invent a `furi_*`, `canvas_*`, `view_*`, or `gui_*` call. If step 1 does
not find it, it does not exist — say so instead of guessing a near-miss name.

## Constraints that bite

- **Heap is small** (order of hundreds of KB, shared with the OS). Allocate
  once at startup, free everything in the teardown path, never allocate in a
  draw callback.
- **Draw callbacks must not block.** No sleeps, no I/O, no allocation. They run
  on the GUI thread.
- **Every `*_alloc` needs its `*_free`** on all exit paths. A leak survives app
  exit and degrades the whole device until reboot.
- `stack_size` in `application.fam` is per-app and real. Deep recursion or big
  stack buffers will overflow it silently-ish.

## Build / run loop

    cd apps/<app>
    ufbt                 # build → dist/<app>.fap
    ufbt launch          # upload + run on the attached Flipper
    ufbt cli             # device shell; run `log` to stream FURI_LOG_* output
    ufbt lint            # clang-format check (CI runs this too)
    ufbt format          # rewrite to the SDK's clang-format; run once after
                         #   adding a file, before trusting `ufbt lint`

**Claude cannot see the screen or press buttons.** The serial log is the only
channel back. Instrument with `FURI_LOG_I(TAG, ...)` and ask the human to paste
`ufbt cli` → `log` output. Do not infer that an app works because it compiled;
a clean build only means it links.

## Adding an app

One directory per app under `apps/`, each with its own `application.fam`.
`appid` must be unique across the device. To add an icon, put a 10x10 1-bit PNG
next to the source and set `fap_icon=` — a wrong-format PNG fails the build,
so add it as its own change.
