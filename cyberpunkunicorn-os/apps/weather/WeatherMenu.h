#pragma once

#include <MenuItemInterface.h>

// "Weather": a synthwave weather card (Open-Meteo, no API key) with rainbow
// temperatures, pixel-art condition icons and a 3-day strip, plus an Indoor page
// fed by the Sensors app through weather::indoorClimate(). Ported from the
// CircuitPython Unicorn OS launcher's weather pages.
class WeatherMenu : public MenuItemInterface {
public:
    WeatherMenu() : MenuItemInterface("Weather") {}

    void optionsMenu(void) override;
    void drawIcon(float scale) override;
    bool hasTheme() override { return false; }
    const String &themePath() override { return _noTheme; }

private:
    String _noTheme = "";
    void run();
    void locationMenu();
};
