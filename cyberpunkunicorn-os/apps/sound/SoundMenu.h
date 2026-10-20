#pragma once

#include <MenuItemInterface.h>
#include <SimpleCLI.h>

// "Sound": tones and alerts on the CYD's speaker amp input (GPIO26), as an
// LEDC square wave. Volume is the duty cycle (5 steps, saved in NVS "cybersnd").
class SoundMenu : public MenuItemInterface {
public:
    SoundMenu() : MenuItemInterface("Sound") {}

    void optionsMenu(void) override;
    void drawIcon(float scale) override;
    bool hasTheme() override { return false; }
    const String &themePath() override { return _noTheme; }

private:
    String _noTheme = "";
};

// Serial CLI: `tone <hz> [ms] [vol 0-4]`, `tone jingle`, `tone sweep`,
// `tone alert|ok|error`.
void createSoundCommand(SimpleCLI *cli);
