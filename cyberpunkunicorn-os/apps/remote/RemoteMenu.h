#pragma once

#include <MenuItemInterface.h>
#include <SimpleCLI.h>

// "Remote" main-menu app: phone control (Frequency Experience) over USB and
// Bluetooth. Shows the connection state and recent commands, and switches the
// Bluetooth remote on/off now and at boot. Protocol: apps/remote/PROTOCOL.md.
class RemoteMenu : public MenuItemInterface {
public:
    RemoteMenu() : MenuItemInterface("Remote") {}

    void optionsMenu(void) override;
    void drawIcon(float scale) override;
    bool hasTheme() override { return false; }
    const String &themePath() override { return _noTheme; }

private:
    String _noTheme = "";
    void showStatus();
    void showHelp();
};

// Serial CLI: `remote` prints the same status; `remote ble on|off`, `remote boot on|off`.
void createRemoteCommand(SimpleCLI *cli);
