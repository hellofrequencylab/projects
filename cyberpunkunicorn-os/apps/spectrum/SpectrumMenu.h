#pragma once

#include <MenuItemInterface.h>
#include <SimpleCLI.h>

// "Spectrum": one analyzer screen for every band the CYD can hear.
//   Wi-Fi     2.4 GHz channels 1-13 from a Wi-Fi scan (built in, always works)
//   2.4 GHz   126 x 1 MHz from the nRF24 (hat DIP 2)
//   Sub-GHz   CC1101 RSSI sweeps, presets 315 / 433 / 868 / 915 and full bands (hat DIP 1)
//   Audio     FFT of an analog mic on GPIO35, 0-4 kHz
// Live trace with peak hold, a scrolling waterfall and the loudest frequency.
// NEXT/PREV change the band, SEL clears the peaks, ESC goes back.
class SpectrumMenu : public MenuItemInterface {
public:
    SpectrumMenu() : MenuItemInterface("Spectrum") {}

    void optionsMenu(void) override;
    void drawIcon(float scale) override;
    bool hasTheme() override { return false; }
    const String &themePath() override { return _noTheme; }

private:
    String _noTheme = "";
};

// Serial CLI: `spectrum` prints the last sweep (while the Spectrum screen is open).
void createSpectrumCommand(SimpleCLI *cli);
