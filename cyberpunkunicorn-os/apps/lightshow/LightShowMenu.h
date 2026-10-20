#pragma once

#include <MenuItemInterface.h>

// "Light Show": 18 animated scenes for an 8x8 WS2812B panel, mirrored live on the
// screen (so it works with no panel attached too). Ported from the CircuitPython
// Unicorn OS launcher.
class LightShowMenu : public MenuItemInterface {
public:
    LightShowMenu() : MenuItemInterface("Light Show") {}

    void optionsMenu(void) override;
    void drawIcon(float scale) override;
    bool hasTheme() override { return false; }
    const String &themePath() override { return _noTheme; }

private:
    String _noTheme = "";
    void run();
    void pinMenu();
};
