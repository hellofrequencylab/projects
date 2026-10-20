---
name: cyberunicorn-5000-design
description: Use this skill to generate well-branded interfaces and assets for CyberUnicorn 5000, either for production or throwaway prototypes/mocks/etc. Contains essential design guidelines, colors, type, fonts, assets, and UI kit components for protoyping.
user-invocable: true
---

Read the README.md file within this skill, and explore the other available files.
If creating visual artifacts (slides, mocks, throwaway prototypes, etc), copy assets out and create static HTML files for the user to view. If working on production code, you can copy assets and read the rules here to become an expert in designing with this brand.
If the user invokes this skill without any other guidance, ask them what they want to build or design, ask some questions, and act as an expert designer who outputs HTML artifacts _or_ production code, depending on the need.

For firmware work, the app icons ship as `apps/ui/cyber_icons.h` (run-length data, drawn by `apps/ui/cyber_icon.cpp`); edit shapes in `icons/build_icons.js`, run `node icons/gen_firmware.js` and never hand-edit the header. What changed when the system landed in firmware: `CHANGES.md`.
