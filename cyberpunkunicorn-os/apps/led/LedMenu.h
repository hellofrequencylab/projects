#pragma once

#include <MenuItemInterface.h>
#include <SimpleCLI.h>

// "LED": the CYD's onboard RGB LED (pins 4/16/17, active LOW) on LEDC PWM.
// A picked colour stays lit after leaving the app; the animations stop on exit.
class LedMenu : public MenuItemInterface {
public:
    LedMenu() : MenuItemInterface("LED") {}

    void optionsMenu(void) override;
    void drawIcon(float scale) override;
    bool hasTheme() override { return false; }
    const String &themePath() override { return _noTheme; }

private:
    String _noTheme = "";
};

// Set the LED (0-255 per channel; all zero releases the pins). Used by the CLI.
void cydLedSet(uint8_t r, uint8_t g, uint8_t b);

// Serial CLI: `led <r> <g> <b>` (0-255) or `led off`.
void createLedCommand(SimpleCLI *cli);
