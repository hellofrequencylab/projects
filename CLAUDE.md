# Repo index

This is a **multi-project repo**. This file is a router, not a rulebook. Each
project directory owns its conventions, and **the project's `CLAUDE.md` wins**
over anything here.

| Working in… | Read first |
|---|---|
| `flipper/` | [`flipper/CLAUDE.md`](flipper/CLAUDE.md) — Flipper Zero / ufbt. Non-negotiable: the SDK is the source of truth, not your training data. |
| a new project | [`_template/CLAUDE.md`](_template/CLAUDE.md), then `README.md` → "Adding a project" |

## Rules that hold everywhere

- **Never cross project boundaries in one change.** One project per PR, one
  project per commit. These projects share a repo, not a codebase.
- **Never add a root-level build, lint, or dependency file.** No root
  `package.json`, `Makefile`, or lockfile. If two projects need the same tool,
  they each declare it.
- **Verify before claiming.** Run the project's own build and say what it
  printed. "Should work" is not a result. For anything cross-compiled, a clean
  build is the minimum bar and is still not proof it runs on hardware.
- **Toolchain drift is the default failure.** For any SDK-backed project, grep
  the installed headers before using an API. Details live in the project file.
