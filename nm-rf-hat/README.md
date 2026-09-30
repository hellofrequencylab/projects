# nm-rf-hat

Custom [Bruce](https://github.com/BruceDevices/firmware) 1.16.1 for a Cheap
Yellow Display (ESP32-2432S028) on an NMTech NM-RF-HAT (CC1101, nRF24, PN532,
IR, 433 MHz OOK). The only change from stock so far: the boot screen text is
configurable.

    # 1. set your text
    $EDITOR boot.conf
    # 2. build (first run installs PlatformIO + toolchain, ~10 min)
    ./build.sh
    # 3. flash over USB (keeps your Bruce settings)
    ./flash.sh

Cable: **USB-A → USB-C into the CYD's own port.** USB-C ↔ USB-C won't power it.

Patches to Bruce live in `patches/`; see `CLAUDE.md` for how to add one.
