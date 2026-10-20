#include "RemoteMenu.h"

#include "../ui/cyber_icon.h"

#include "../ui/rainbow.h"
#include "core/display.h"
#include "core/scrollableTextArea.h"
#include "core/utils.h"
#include "remote.h"
#include <globals.h>

namespace {

String onOff(bool on) { return on ? "ON" : "OFF"; }

String bleLine() {
    if (!remote::bleEnabled()) return "off";
    int n = remote::bleConnected();
    return n > 0 ? "connected (" + String(n) + ")" : "advertising as CU5K";
}

String ago(uint32_t ms) {
    uint32_t s = (millis() - ms) / 1000;
    if (s < 60) return String(s) + "s";
    if (s < 3600) return String(s / 60) + "m";
    return String(s / 3600) + "h";
}

void setBle(bool on) {
    if (!remote::bleSetEnabled(on)) displayWarning("Not enough free RAM for Bluetooth", true);
}

} // namespace

void RemoteMenu::optionsMenu() {
    options = {
        {"Status", [this]() { showStatus(); }},
        {("Bluetooth now: " + onOff(remote::bleEnabled())).c_str(), []() { setBle(!remote::bleEnabled()); }},
        {("Bluetooth at boot: " + onOff(remote::bleAtBoot())).c_str(),
         []() { remote::setBleAtBoot(!remote::bleAtBoot()); }},
        {"How to connect", [this]() { showHelp(); }},
    };
    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_SUBMENU, "Remote");
}

// Live status: redraws only what changed, every half second, until ESC/back.
void RemoteMenu::showStatus() {
    drawMainBorderWithTitle("REMOTE");
    const int x = 14, w = tftWidth - 28;
    int y = 34;
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
    tft.drawString("SEL: Bluetooth on/off   ESC: back", x, tftHeight - 18, 1);

    uint32_t lastCount = UINT32_MAX, lastDraw = 0;
    String lastBle;
    while (true) {
        if (check(EscPress)) break;
        if (check(SelPress)) {
            setBle(!remote::bleEnabled());
            drawMainBorderWithTitle("REMOTE");
            tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
            tft.drawString("SEL: Bluetooth on/off   ESC: back", x, tftHeight - 18, 1);
            lastCount = UINT32_MAX;
        }
        if (millis() - lastDraw < 500) {
            delay(20);
            continue;
        }
        lastDraw = millis();

        String ble = bleLine();
        uint32_t count = remote::commandCount();
        tft.setTextSize(1);
        // Connection block (heap changes constantly, so it's always redrawn).
        tft.fillRect(x, y, w, 40, bruceConfig.bgColor);
        tft.setTextColor(cyberHue(180), bruceConfig.bgColor);
        tft.drawString("USB  serial 115200, always on", x, y, 1);
        tft.setTextColor(cyberHue(280), bruceConfig.bgColor);
        tft.drawString("BLE  " + ble + (remote::bleAtBoot() ? "  [boot]" : ""), x, y + 12, 1);
        tft.setTextColor(cyberHue(40), bruceConfig.bgColor);
        tft.drawString(
            "RAM  " + String(ESP.getFreeHeap() / 1024) + " KB free, " + String(count) + " cmds", x, y + 24, 1
        );

        if (count != lastCount || ble != lastBle) {
            lastCount = count;
            lastBle = ble;
            int ly = y + 46;
            tft.fillRect(x, ly, w, tftHeight - 22 - ly, bruceConfig.bgColor);
            cyberRainbowText("RECENT", x, ly, cyberPhase(), bruceConfig.bgColor);
            remote::LogEntry e[8];
            int n = remote::recent(e, 8);
            int maxRows = (tftHeight - 24 - (ly + 14)) / 12;
            if (n == 0) {
                tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
                tft.drawString("no commands yet", x, ly + 14, 1);
            }
            for (int i = 0; i < n && i < maxRows; i++) {
                tft.setTextColor(e[i].ok ? cyberHue(i * 40) : TFT_RED, bruceConfig.bgColor);
                String row = String(e[i].transport) + " " + String(e[i].cmd) + (e[i].ok ? "" : " !") +
                             "  " + ago(e[i].ms);
                tft.drawString(row, x, ly + 14 + i * 12, 1);
            }
        }
    }
    returnToMenu = true;
}

void RemoteMenu::showHelp() {
    ScrollableTextArea area("REMOTE");
    area.addLine("Phone app: Frequency Experience");
    area.addLine("");
    area.addLine("USB: OTG cable to the CYD port,");
    area.addLine("115200 8N1, DTR+RTS OFF");
    area.addLine("(asserting them resets the board)");
    area.addLine("");
    area.addLine("Bluetooth: turn it on here,");
    area.addLine("then scan for CU5K.");
    area.addLine("Needs a lot of RAM: Bluetooth");
    area.addLine("apps can't run while it's on.");
    area.addLine("");
    area.addLine("Protocol: apps/remote/PROTOCOL.md");
    area.show();
}

void RemoteMenu::drawIcon(float scale) {
    // Pixel-art icon from design/icons (apps/ui/cyber_icons.h).
    clearIconArea();
    cyberIcon(getName().c_str(), iconCenterX, iconCenterY, max(2, (int)(scale * 2.4f + 0.5f)), true);
}

static uint32_t remoteCallback(cmd *c) {
    Command cmd(c);
    String what = cmd.getArgument("what").getValue();
    String value = cmd.getArgument("value").getValue();
    what.trim();
    value.trim();
    if (what == "ble" && (value == "on" || value == "off")) {
        if (!remote::bleSetEnabled(value == "on")) serialDevice->println("Not enough free RAM for Bluetooth");
    } else if (what == "boot" && (value == "on" || value == "off")) {
        remote::setBleAtBoot(value == "on");
    } else if (what.length()) {
        serialDevice->println("usage: remote [ble on|off] [boot on|off]");
        return false;
    }
    serialDevice->println("Bluetooth: " + bleLine() + ", at boot " + onOff(remote::bleAtBoot()));
    serialDevice->println("Free heap: " + String(ESP.getFreeHeap()) + " bytes");
    serialDevice->println("Commands: " + String(remote::commandCount()));
    remote::LogEntry e[8];
    int n = remote::recent(e, 8);
    for (int i = 0; i < n; i++)
        serialDevice->println(
            "  " + String(e[i].transport) + " " + e[i].cmd + (e[i].ok ? " ok " : " FAIL ") + ago(e[i].ms)
        );
    return true;
}

void createRemoteCommand(SimpleCLI *cli) {
    Command c = cli->addCommand("remote", remoteCallback);
    c.addPosArg("what", "");
    c.addPosArg("value", "");
}
