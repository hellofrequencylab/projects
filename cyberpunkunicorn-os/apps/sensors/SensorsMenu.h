#pragma once

#include <MenuItemInterface.h>
#include <SimpleCLI.h>

// "Sensors": hub for external sensors. Wired I2C chips on the Grove port and
// ESP-NOW sensor nodes (sensor_packet.h) in one live list, a big-number view with
// a sparkline, an altimeter, and CSV logging to SD or LittleFS.
class SensorsMenu : public MenuItemInterface {
public:
    SensorsMenu() : MenuItemInterface("Sensors") {}

    void optionsMenu(void) override;
    void drawIcon(float scale) override;
    bool hasTheme() override { return false; }
    const String &themePath() override { return _noTheme; }

private:
    String _noTheme = "";
};

// Serial CLI: `sensors` (current readings), `sensors scan`, `sensors listen [s]`.
void createSensorsCommand(SimpleCLI *cli);
