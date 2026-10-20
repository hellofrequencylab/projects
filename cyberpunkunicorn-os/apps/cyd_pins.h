// The CYD's own (non-hat) extras, as used by our LED / Light / Sound / GPIO
// apps. None of these pins is driven by Bruce's CYD-2432S028 build (it has
// RGB_LED=-1, no BUZZ_PIN, no speaker, no LDR). Values are the common
// ESP32-2432S028 schematic, not yet confirmed on our unit: see SCOPE.md.
#pragma once

// Onboard RGB LED, common anode: a pin LOW turns its colour ON.
constexpr int CYD_LED_R = 4;
constexpr int CYD_LED_G = 16;
constexpr int CYD_LED_B = 17;
constexpr bool CYD_LED_ACTIVE_LOW = true; // flip if colours come out inverted

// LDR divider into ADC1_CH6 (input-only pin).
constexpr int CYD_LDR = 34;
constexpr bool CYD_LDR_BRIGHT_LOW = true; // more light -> lower reading; flip if backwards

// Speaker amp input (also DAC2). Pin 25 (DAC1) is touch CLK: never use it.
constexpr int CYD_SPEAKER = 26;
