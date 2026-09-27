# AIO Board V1.4 — hardware canon

Expansion board on the Flipper Zero GPIO header. **Everything in this file was
read off the physical board**, not inferred from a product listing. When code
and this file disagree, check the board again and fix this file in the same
change.

## What is actually on it

| Function | Part | Notes |
|---|---|---|
| WiFi | **ESP32-S2-SOLO-2U** (FCC 2AC7Z-ESPS2SOLO2U) | Single core, **no Bluetooth radio at all**, native USB, external antenna (`-U`) |
| Sub-GHz | **EBYTE E07-433M20S** (CC1101 + 20 dBm PA) | **433 MHz only** |
| 2.4 GHz | **Ashining A01 S2G4A20S2a** (nRF24L01+ PA/LNA) | 20 dBm |
| Storage | microSD slot | For the ESP32 (pcap, Evil Portal assets) |
| Power/data | USB-C → ESP32-S2 | Independent of the Flipper's own USB-C |

Silkscreen reads **AIO BOARD V1.4**. Do not write "V4.1" anywhere.

## Three constraints that will bite

1. **The front switch is a hardware mux — one module at a time.** It selects
   NRF24 / ESP32 / CC1101, and the LED below reports which: **red = NRF24,
   green = ESP32/WiFi, blue = CC1101**. There is no software override. Any app
   that wants WiFi *and* sub-GHz in one flow is not possible on this board;
   design for one radio per mode and make the user flip the switch.
2. **No Bluetooth.** The S2 has no BT radio. Every BLE feature in Marauder
   (BLE scan, BLE spam, sour-apple, etc.) is unavailable — not broken, absent.
   Do not write code paths for it and do not report it as a bug.
3. **The CC1101 is 433 MHz only.** The E07's matching network and PA are cut
   for 433. 315 / 868 / 915 MHz will not work correctly through this board even
   though the Flipper's own sub-GHz stack offers those frequencies.

## Switches and buttons

- **Front function switch** — selects the active module (see mux above).
  **Middle position is also what download mode needs.**
- **Back RX/TX switch** — CC1101 gain direction, *not* a UART swap:
  `RX` engages receive gain (LNA), `TX` engages transmit gain (PA).
- **RT** = reset. **BT** = boot (download mode).

## Flashing the ESP32-S2

    1. Front function switch → MIDDLE position
    2. Hold BT
    3. Plug the board's USB-C into the computer (keep holding BT)
    4. Release BT. Front LED should go green.
    5. Target chip is esp32-s2 — never esp32 or esp32-s3

On macOS the board enumerates on `/dev/cu.*` (S2 native USB). Confirm before
flashing anything:

    esptool --port /dev/cu.<board> chip_id     # must report ESP32-S2

## Pin headers

- **Flipper GPIO**: pins numbered 1-18 along the board's bottom edge, matching
  the Flipper's own header numbering.
- **"ESP32 LEADS OUT PINS"**: `3V3 3V3 GND GND TX RX 38 37 36 35` — the 35-38
  are ESP32-S2 GPIO numbers, free for your own firmware.
- Separate 4-pin `3V3 GND RX TX` breakout for the ESP32 UART.

## Transmit power

Both the 433 MHz and 2.4 GHz front ends are 20 dBm (100 mW) amplified parts.
That is well above the limit for unlicensed ISM operation in most regions, and
the 433 MHz band in particular is allocated differently country to country.
Receive freely; know what the rules are where you are before keying up, and
keep TX to hardware you own.
