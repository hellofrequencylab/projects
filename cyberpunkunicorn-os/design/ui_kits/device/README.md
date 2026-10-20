# Device UI kit

Click-through of the CyberUnicorn 5000 on its 320x240 CYD screen, shown at 2x.

- `index.html` — boot scene → main menu (all 23 icons) → app launch screen. D-pad on the right or arrow keys / Enter / Esc.
- `AppScreen.jsx` — shared launch frame: rainbow title bar, icon, option list in a RainbowFrame.

Sources: boot scene is a faithful port of `apps/boot/unicorn_boot.cpp`. The main-menu layout approximates Bruce 1.16.1 (centred icon + label) with the patch 0008 rainbow skin; Bruce's source was not available to copy exact geometry. App option lists are taken from each app's description in `cyberpunkunicorn-os/CLAUDE.md`; the real in-app screens are not recreated.
