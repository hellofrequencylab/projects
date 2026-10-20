# CyberUnicorn 5000™

Firmware for a Cheap Yellow Display (ESP32-2432S028) on an NMTech NM-RF-HAT
(CC1101, nRF24, PN532, IR, 433 MHz OOK). Built on
[Bruce](https://github.com/BruceDevices/firmware) 1.16.1, with our branding,
a glitchy synthwave unicorn boot screen, a rainbow skin on every screen and the
web UI, and over time an app for every function of the boards plus external sensors.
Roadmap: [`SCOPE.md`](SCOPE.md).

    # 1. optional: edit the boot text
    $EDITOR boot.conf
    # 2. build (first run installs PlatformIO + toolchain, ~10 min)
    ./build.sh
    # 3. flash over USB (keeps on-device settings)
    ./flash.sh

Cable: **USB-A → USB-C into the CYD's own port.** USB-C ↔ USB-C won't power it,
and the hat's port gives power but no data.

## Remote screen over USB

    ./remote.sh

Mirrors the screen to http://localhost:8765 with buttons and a serial console.
Only one program can hold the serial port, so quit it (Ctrl-C) before `./flash.sh`.

## License

Bruce is AGPL-3.0, so this firmware is too: if you distribute builds, publish
the source (this directory + the Bruce tag it pins).
