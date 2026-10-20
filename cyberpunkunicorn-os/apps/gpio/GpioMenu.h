#pragma once

#include <MenuItemInterface.h>
#include <SimpleCLI.h>

// "GPIO": on-screen pin tool (read input, toggle output, PWM) for the only pins
// that are free on CYD + NM-RF-HAT: 22 and 27 (shared with every hat module,
// so DIP 1-5 must be OFF) and 35 (input only). TFT, touch, SD, flash, USB
// serial and the onboard LED/LDR/speaker pins are never offered.
class GpioMenu : public MenuItemInterface {
public:
    GpioMenu() : MenuItemInterface("GPIO") {}

    void optionsMenu(void) override;
    void drawIcon(float scale) override;
    bool hasTheme() override { return false; }
    const String &themePath() override { return _noTheme; }

private:
    String _noTheme = "";
};

// Serial CLI: `pins` lists the free pins with their level and current owner.
// (Bruce's own `gpio mode/set/read` commands stay as they are.)
void createPinsCommand(SimpleCLI *cli);
