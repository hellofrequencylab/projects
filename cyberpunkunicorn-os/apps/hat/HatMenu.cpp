#include "HatMenu.h"

#include "../ui/cyber_icon.h"

#include "core/display.h"
#include "core/scrollableTextArea.h"
#include "core/utils.h"
#include "hat_probe.h"
#include <SimpleCLI.h>
#include <globals.h>

void HatMenu::optionsMenu() {
    options = {
        {"Detect modules",   [this]() { showDetect(); }  },
        {"DIP switch guide", [this]() { showDipGuide(); }},
    };
    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_SUBMENU, "Hat");
}

void HatMenu::showDetect() {
    displayInfo("Probing hat...");
    HatProbeResult r = probeHat();

    ScrollableTextArea area("HAT MODULES");
    for (const String &line : hatReportLines(r)) area.addLine(line);
    area.show();
}

void HatMenu::showDipGuide() {
    ScrollableTextArea area("DIP SWITCH");
    area.addLine("Only ONE of 1-5 ON at a time.");
    area.addLine("They all share pins 22/27.");
    area.addLine("");
    area.addLine("1  CC1101 sub-GHz  -> RF");
    area.addLine("2  nRF24 2.4 GHz   -> NRF24");
    area.addLine("3  PN532 NFC       -> RFID");
    area.addLine("4  IR TX/RX        -> IR");
    area.addLine("5  433 MHz OOK     -> RF");
    area.addLine("6  battery power");
    area.addLine("");
    area.addLine("I2C sensors on Grove can share");
    area.addLine("the bus with 3 (PN532) only.");
    area.show();
}

void HatMenu::drawIcon(float scale) {
    // Pixel-art icon from design/icons (apps/ui/cyber_icons.h).
    clearIconArea();
    cyberIcon(getName().c_str(), iconCenterX, iconCenterY, max(2, (int)(scale * 2.4f + 0.5f)), true);
}

static uint32_t hatCallback(cmd *c) {
    HatProbeResult r = probeHat();
    for (const String &line : hatReportLines(r)) serialDevice->println(line);
    return true;
}

void createHatCommand(SimpleCLI *cli) { cli->addCommand("hat", hatCallback); }
