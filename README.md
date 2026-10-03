# projects

One repo, many unrelated programming projects. Each top-level directory is a
self-contained project: its own toolchain, its own build command, its own
`CLAUDE.md`. Nothing at the root knows how to build anything.

| Project | What it is | Build |
|---|---|---|
| [`flipper/`](flipper/) | Flipper Zero apps (`.fap`, C, ufbt) | `cd flipper/apps/<app> && ufbt` |
| [`aio-board/`](aio-board/) | AIO Board V1.4 expansion hardware (ESP32-S2, CC1101, nRF24) | firmware TBD; hardware canon only |
| [`nm-rf-hat/`](nm-rf-hat/) | Custom Bruce firmware for CYD (ESP32-2432S028) + NM-RF-HAT | `cd nm-rf-hat && ./build.sh` |
| [`cybermesh/`](cybermesh/) | Mesh system across the repo's devices (brainstorm) | none yet; stack undecided |

## Adding a project

    cp -r _template my-thing && cd my-thing

Then fill in `my-thing/CLAUDE.md` and add a row to the table above. Rules:

1. **Self-contained.** A project builds from inside its own directory. No root
   build script, no shared lockfile, no cross-project imports.
2. **Its own `CLAUDE.md`.** Claude Code loads the nearest `CLAUDE.md` above the
   file being edited, so per-project files are what keep a polyglot repo
   workable — one root file would be 90% irrelevant noise on any given task.
   The root file routes; it never restates.
3. **Its own CI job, path-filtered.** Touching `flipper/` must not run a Rust
   build. See `.github/workflows/`.
4. **Pin the toolchain version** in the project's `CLAUDE.md`. Embedded and
   systems SDKs break across releases; "latest" is not a version.
