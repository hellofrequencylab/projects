#pragma once

#include <MenuItemInterface.h>
#include <SimpleCLI.h>

// "Hat" main-menu app: detects which NM-RF-HAT module is live and shows the
// DIP switch map.
class HatMenu : public MenuItemInterface {
public:
    HatMenu() : MenuItemInterface("Hat") {}

    void optionsMenu(void) override;
    void drawIcon(float scale) override;
    bool hasTheme() override { return false; }
    const String &themePath() override { return _noTheme; }

private:
    String _noTheme = "";
    void showDetect();
    void showDipGuide();
};

// Serial CLI: `hat` prints the same report as the screen.
void createHatCommand(SimpleCLI *cli);
