#pragma once

#include <MenuItemInterface.h>
#include <SimpleCLI.h>

// "Light": light meter on the CYD's LDR (GPIO34, ADC1). Live reading, a big
// rainbow bar and a scrolling history graph.
class LightMenu : public MenuItemInterface {
public:
    LightMenu() : MenuItemInterface("Light") {}

    void optionsMenu(void) override;
    void drawIcon(float scale) override;
    bool hasTheme() override { return false; }
    const String &themePath() override { return _noTheme; }

private:
    String _noTheme = "";
};

// Serial CLI: `light` prints raw ADC, millivolts and the light percentage.
void createLightCommand(SimpleCLI *cli);
