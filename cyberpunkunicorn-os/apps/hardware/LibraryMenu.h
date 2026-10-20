#pragma once

#include <MenuItemInterface.h>
#include <SimpleCLI.h>

// "Library": every app whose hardware hasn't been found yet (hardware/hardware.h).
// Lists what each one needs and how to connect it; "I have this" or a successful
// probe moves the app up to the main menu.
class LibraryMenu : public MenuItemInterface {
public:
    LibraryMenu() : MenuItemInterface("Library") {}

    void optionsMenu(void) override;
    void drawIcon(float scale) override;
    bool hasTheme() override { return false; }
    const String &themePath() override { return _noTheme; }

private:
    String _noTheme = "";
    void appMenu(MenuItemInterface *item);
    void showNeeds(MenuItemInterface *item);
    void showHardware();
    void rescanNow();
};

// Main menu filter (patch 0016): false = the app waits in the Library.
bool cyberAppActive(const String &menuName);

// Called by the main menu (patch 0016) before opening an app. For apps that need a
// hat module, re-probes and, if the right module isn't on, shows the DIP switch
// card. Returns false if the user backed out.
bool cyberHardwareGate(MenuItemInterface *item);

// Serial CLI: `hw`, `hw rescan`, `hw have <part>`, `hw nothave <part>`, `hw forget <part>|all`.
void createHardwareCommand(SimpleCLI *cli);
