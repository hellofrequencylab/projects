#pragma once

#include <MenuItemInterface.h>
#include <SimpleCLI.h>

// "Frequency" main-menu app: the NFC side of the Frequency loyalty platform
// (frequency-web). Programs and verifies venue check-in / event door tags on
// the hat's PN532, and, in reader mode, scans member cards against the
// proposed device API (BACKEND.md).
class FrequencyMenu : public MenuItemInterface {
public:
    FrequencyMenu() : MenuItemInterface("Frequency") {}

    void optionsMenu(void) override;
    void drawIcon(float scale) override;
    bool hasTheme() override { return false; }
    const String &themePath() override { return _noTheme; }

private:
    String _noTheme = "";
};

// Serial CLI: `frequency read|tag|tags|log|scan|issue|queue|config|status`.
void createFrequencyCommand(SimpleCLI *cli);
