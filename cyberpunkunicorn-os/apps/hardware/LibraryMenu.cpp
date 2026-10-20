#include "LibraryMenu.h"

#include "core/display.h"
#include "core/main_menu.h"
#include "core/scrollableTextArea.h"
#include "core/utils.h"
#include "../ui/scene.h"
#include "hardware.h"
#include <globals.h>
#include <vector>

namespace {

// Main-menu items that wait in the Library right now, in menu order.
std::vector<MenuItemInterface *> inactiveApps() {
    std::vector<MenuItemInterface *> out;
    for (MenuItemInterface *item : mainMenu.getItems())
        if (!hw::appActive(item->getName())) out.push_back(item);
    return out;
}

// "NFC reader (PN532)" or "Sub-GHz radio (CC1101) or 433 MHz module".
String needsLabel(const hw::AppNeed &n) {
    String s;
    for (int i = 0; i < hw::HW_PART_COUNT; i++) {
        if (!(n.anyOf & hw::partBit(hw::Part(i)))) continue;
        if (s.length()) s += " or ";
        s += hw::info(hw::Part(i)).name;
    }
    return s;
}

String stateLabel(hw::Part p) {
    if (hw::liveNow(p)) return "on now";
    if (hw::detected(p)) return "found before";
    if (hw::marked(p)) return "you said yes";
    return hw::info(p).autoDetect ? "not found" : "not set";
}

// The design-system hardware card for an app whose parts are all hat modules:
// "TURN DIP n ON" for the first one it can use. 0 Check again, 1 Open anyway, 2 Back.
int hatCard(const String &app) {
    const hw::AppNeed *n = hw::needFor(app);
    if (!n) return 1;
    String module, pins;
    int dip = 0;
    for (int i = 0; i < hw::HW_PART_COUNT; i++) {
        hw::Part p = hw::Part(i);
        if (!(n->anyOf & hw::partBit(p)) || !hw::info(p).dip) continue;
        if (!dip) {
            dip = hw::info(p).dip;
            module = hw::info(p).name;
            pins = hw::info(p).howTo;
        } else {
            pins += " (or DIP " + String(hw::info(p).dip) + ": " + hw::info(p).name + ")";
        }
    }
    if (!dip) return 1;
    uint8_t on = 1u << (dip - 1);
    uint8_t off = 0x1F & ~on; // switch 6 is the battery: left alone
    return cyberHardwareCard(app, "TURN DIP " + String(dip) + " ON", module, pins, on, off);
}

} // namespace

bool cyberAppActive(const String &menuName) { return hw::appActive(menuName); }

bool cyberHardwareGate(MenuItemInterface *item) {
    String name = item->getName();
    if (!hw::appNeedsHatModule(name)) return true;
    if (!hw::rescan() || hw::appReadyNow(name)) return true; // busy: don't block the app
    for (;;) {
        int choice = hatCard(name);
        if (choice == 1) return true;
        if (choice != 0) return false;
        displayInfo("Checking hardware...");
        if (hw::rescan() && hw::appReadyNow(name)) return true;
    }
}

void LibraryMenu::optionsMenu() {
    std::vector<MenuItemInterface *> apps = inactiveApps();
    options = {
        {"Check hardware now", [this]() { rescanNow(); }},
    };
    for (MenuItemInterface *item : apps) {
        String label = item->getName();
        options.push_back({label.c_str(), [this, item]() { appMenu(item); }});
    }
    if (apps.empty()) options.push_back({"All apps are active", [this]() { optionsMenu(); }});
    options.push_back({"My hardware", [this]() { showHardware(); }});
    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_SUBMENU, "Library");
}

void LibraryMenu::rescanNow() {
    displayInfo("Checking hardware...");
    size_t before = inactiveApps().size();
    if (!hw::rescan()) {
        displayWarning("Stop Light Show first: it is using the hat pins.", true);
    } else {
        size_t after = inactiveApps().size();
        if (after < before) displaySuccess(String(before - after) + " app(s) moved to the main menu", true);
        else displayInfo("Nothing new found. Check the DIP switch.", true);
    }
    optionsMenu();
}

void LibraryMenu::appMenu(MenuItemInterface *item) {
    const hw::AppNeed *n = hw::needFor(item->getName());
    if (!n) return optionsMenu();
    options = {
        {"What it needs", [this, item]() { showNeeds(item); }},
    };
    // One "I have" line per part the probe can't see for itself.
    for (int i = 0; i < hw::HW_PART_COUNT; i++) {
        hw::Part p = hw::Part(i);
        if (!(n->anyOf & hw::partBit(p)) || hw::info(p).autoDetect) continue;
        String label = String("I have: ") + hw::info(p).name;
        options.push_back({label.c_str(), [this, p, item]() {
                               hw::setMarked(p, true);
                               displaySuccess(item->getName() + " is now in the main menu", true);
                               optionsMenu();
                           }});
    }
    options.push_back({"Open anyway", [item]() { item->optionsMenu(); }});
    options.push_back({"Back", [this]() { optionsMenu(); }});
    loopOptions(options, MENU_TYPE_SUBMENU, item->getName().c_str());
}

void LibraryMenu::showNeeds(MenuItemInterface *item) {
    const hw::AppNeed *n = hw::needFor(item->getName());
    if (hw::appNeedsHatModule(item->getName())) {
        // Hat modules get the drawn DIP switch card.
        for (;;) {
            int choice = hatCard(item->getName());
            if (choice == 1) return item->optionsMenu();
            if (choice != 0) return appMenu(item);
            displayInfo("Checking hardware...");
            hw::rescan();
            if (hw::appActive(item->getName())) {
                displaySuccess(item->getName() + " is now in the main menu", true);
                return optionsMenu();
            }
        }
    }
    ScrollableTextArea area(item->getName());
    if (n) {
        area.addLine("Needs " + needsLabel(*n) + ".");
        area.addLine("");
        for (int i = 0; i < hw::HW_PART_COUNT; i++) {
            hw::Part p = hw::Part(i);
            if (!(n->anyOf & hw::partBit(p))) continue;
            area.addLine(String(hw::info(p).name) + ":");
            area.addLine("  " + String(hw::info(p).howTo));
            area.addLine(hw::info(p).autoDetect ? "  Found automatically. Then: Check hardware now."
                                                : "  Can't be detected. Pick \"I have\" once it's connected.");
        }
        area.addLine("");
        area.addLine("Only one of hat DIP 1 to 5 can be ON.");
    }
    area.show();
    appMenu(item);
}

void LibraryMenu::showHardware() {
    ScrollableTextArea area("MY HARDWARE");
    for (int i = 0; i < hw::HW_PART_COUNT; i++) {
        hw::Part p = hw::Part(i);
        area.addLine(String(hw::info(p).name) + ": " + stateLabel(p));
    }
    area.addLine("");
    area.addLine("Found parts stay found when you flip");
    area.addLine("the DIP switch. Forget: `hw forget all`.");
    area.show();
    optionsMenu();
}

void LibraryMenu::drawIcon(float scale) {
    clearIconArea();
    // Three books on a shelf, the last one leaning.
    int bw = scale * 12, bh = scale * 40, gap = scale * 4;
    int baseY = iconCenterY + bh / 2;
    int x = iconCenterX - (3 * bw + 2 * gap) / 2;
    for (int i = 0; i < 2; i++) {
        int bx = x + i * (bw + gap);
        tft.drawRect(bx, baseY - bh, bw, bh, bruceConfig.priColor);
        tft.drawFastHLine(bx + 2, baseY - bh + bh / 4, bw - 4, bruceConfig.priColor);
    }
    int lx = x + 2 * (bw + gap);
    tft.drawLine(lx, baseY, lx + bh / 3, baseY - bh + 2, bruceConfig.priColor);
    tft.drawLine(lx + bw, baseY, lx + bw + bh / 3, baseY - bh + 2, bruceConfig.priColor);
    tft.drawLine(lx + bh / 3, baseY - bh + 2, lx + bw + bh / 3, baseY - bh + 2, bruceConfig.priColor);
    tft.drawFastHLine(x - gap, baseY, 3 * bw + 4 * gap + bh / 3, bruceConfig.priColor);
}

// ---- CLI ---------------------------------------------------------------------

static void printLine(const String &s) { serialDevice->println(s); }

static uint32_t hardwareCallback(cmd *c) {
    Command cmd(c);
    String action = cmd.getArgument("action").getValue();
    String arg = cmd.getArgument("part").getValue();
    action.trim();
    arg.trim();

    if (action == "rescan") {
        if (!hw::rescan()) {
            printLine("busy: Light Show is using the hat pins");
            return false;
        }
        action = "";
    }
    if (action == "have" || action == "nothave" || action == "forget") {
        hw::Part p;
        if (action == "forget" && arg == "all") {
            hw::forgetAll();
        } else if (!hw::partFromId(arg, p)) {
            String ids;
            for (int i = 0; i < hw::HW_PART_COUNT; i++) ids += String(i ? ", " : "") + hw::info(hw::Part(i)).id;
            printLine("unknown part. Use one of: " + ids);
            return false;
        } else if (action == "forget") hw::forget(p);
        else hw::setMarked(p, action == "have");
        action = "";
    }
    if (action.length()) {
        printLine("usage: hw [rescan | have <part> | nothave <part> | forget <part>|all]");
        return false;
    }

    printLine("Hardware:");
    for (int i = 0; i < hw::HW_PART_COUNT; i++) {
        hw::Part p = hw::Part(i);
        printLine("  " + String(hw::info(p).id) + "  " + hw::info(p).name + ": " + stateLabel(p));
    }
    printLine("Library (waiting for hardware):");
    std::vector<MenuItemInterface *> apps = inactiveApps();
    if (apps.empty()) printLine("  none, every app is in the main menu");
    for (MenuItemInterface *item : apps) {
        const hw::AppNeed *n = hw::needFor(item->getName());
        printLine("  " + item->getName() + ": needs " + (n ? needsLabel(*n) : String("?")));
    }
    return true;
}

void createHardwareCommand(SimpleCLI *cli) {
    Command c = cli->addCommand("hw", hardwareCallback);
    c.addPosArg("action", "");
    c.addPosArg("part", "");
}
