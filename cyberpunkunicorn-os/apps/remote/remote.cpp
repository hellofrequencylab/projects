#include "remote.h"

#include "../hat/hat_probe.h"
#include "../lightshow/LightShowMenu.h"
#include "core/display.h"
#include "core/main_menu.h"
#include "core/settings.h"
#include "core/utils.h"
#include <NimBLEDevice.h>
#include <Preferences.h>
#include <WiFi.h>
#include <algorithm>
#include <globals.h>
#include <vector>

namespace remote {

namespace {

// ---- registry ------------------------------------------------------------------

struct Entry {
    const char *name;
    Handler fn;
};

// Function-local static: registrations run from other files' static initialisers.
std::vector<Entry> &registry() {
    static std::vector<Entry> r;
    return r;
}

Handler find(const String &name) {
    for (const Entry &e : registry())
        if (name == e.name) return e.fn;
    return nullptr;
}

// Protocol v1 commands whose app lives on another branch. Until that app registers
// a handler they answer not_available (not unknown_command), so the phone can grey
// them out instead of treating them as a bug.
const char *const PLANNED[] = {
    "led.set",      "led.mode",           "sound.tone",          "light.read",
    "sensors.read", "frequency.read_tag", "frequency.write_tag", "frequency.scan",
};

bool planned(const String &name) {
    for (const char *p : PLANNED)
        if (name == p) return true;
    return false;
}

// ---- output ----------------------------------------------------------------------

SemaphoreHandle_t writeLock() {
    static SemaphoreHandle_t m = xSemaphoreCreateMutex();
    return m;
}

bool bleSerialActive() { return serialDevice != &USBserial && isBLEAPIEnabled(); }

// Bytes per BLE notification. NimBLE silently truncates a notification to MTU-3,
// and Bruce's BLE serial sends each print() as one notification, so long lines
// must be split here. 0 = no limit (USB).
size_t chunkSize(SerialDevice *dev) {
    if (dev == &USBserial || !bleSerialActive()) return 0;
    uint16_t mtu = 23;
    if (NimBLEDevice::isInitialized()) {
        NimBLEServer *srv = NimBLEDevice::getServer();
        if (srv && srv->getConnectedCount() > 0) mtu = srv->getPeerInfo(0).getMTU();
    }
    return mtu > 3 ? mtu - 3 : 20;
}

void writeTo(SerialDevice *dev, const String &line) {
    size_t chunk = chunkSize(dev);
    if (chunk == 0 || line.length() <= chunk) {
        dev->print(line);
        return;
    }
    for (size_t i = 0; i < line.length(); i += chunk) dev->print(line.substring(i, i + chunk));
}

// One complete line, never interleaved with another of ours.
void writeLine(SerialDevice *dev, const String &line) {
    xSemaphoreTake(writeLock(), portMAX_DELAY);
    writeTo(dev, line);
    xSemaphoreGive(writeLock());
}

// ---- recent-command log ----------------------------------------------------------

constexpr int LOG_SIZE = 8;
LogEntry logRing[LOG_SIZE];
int logHead = 0, logCount = 0;
uint32_t total = 0;
portMUX_TYPE logMux = portMUX_INITIALIZER_UNLOCKED;

void logCommand(const String &cmd, bool ok, SerialDevice *dev) {
    LogEntry e;
    e.ms = millis();
    strlcpy(e.transport, dev == &USBserial ? "usb" : "ble", sizeof(e.transport));
    strlcpy(e.cmd, cmd.c_str(), sizeof(e.cmd));
    e.ok = ok;
    portENTER_CRITICAL(&logMux);
    logRing[logHead] = e;
    logHead = (logHead + 1) % LOG_SIZE;
    if (logCount < LOG_SIZE) logCount++;
    total++;
    portEXIT_CRITICAL(&logMux);
}

// ---- cli.run output capture ------------------------------------------------------

// Stands in for serialDevice while a CLI command runs, collecting what it prints.
// Commands that write to Serial directly (e.g. `nav`'s echo) aren't captured.
class CaptureDevice : public SerialDevice {
public:
    static constexpr size_t LIMIT = 2048;
    String text;
    bool truncated = false;

    size_t println(const String &s) override { return add(s + "\n"); }
    size_t println(size_t n) override { return add(String(n) + "\n"); }
    size_t println(uint32_t n) override { return add(String(n) + "\n"); }
    size_t println() override { return add("\n"); }
    size_t println(int n, int format) override { return add(String(n, format) + "\n"); }
    size_t print(int n, int format) override { return add(String(n, format)); }
    size_t print(const String &s) override { return add(s); }
    void vprintf(const char *fmt, va_list args) override {
        char buf[256];
        vsnprintf(buf, sizeof(buf), fmt, args);
        add(buf);
    }
    size_t write(uint8_t *str, size_t size) override {
        String s;
        s.concat((const char *)str, size);
        return add(s);
    }
    int read() override { return -1; }
    void flush() override {}
    int available() override { return 0; }
    String readStringUntil(char) override { return ""; }

private:
    size_t add(const String &s) {
        if (text.length() + s.length() > LIMIT) {
            truncated = true;
            text += s.substring(0, LIMIT - text.length());
        } else {
            text += s;
        }
        return s.length();
    }
};

// ---- built-in commands -----------------------------------------------------------

void cmdSystemStatus(Request &r) {
    r.data["os"] = OS_NAME;
    r.data["version"] = OS_VERSION;
    r.data["bruce_version"] = BRUCE_VERSION;
    r.data["heap_free"] = ESP.getFreeHeap();
    r.data["heap_min"] = ESP.getMinFreeHeap();
    r.data["uptime_s"] = millis() / 1000;

    JsonObject wifi = r.data["wifi"].to<JsonObject>();
    bool sta = WiFi.status() == WL_CONNECTED;
    wifi["connected"] = sta;
    wifi["ssid"] = sta ? WiFi.SSID() : String("");
    wifi["ip"] = sta ? WiFi.localIP().toString() : String("");
    wifi_mode_t mode = WiFi.getMode();
    if (mode & WIFI_MODE_AP) wifi["ap_ip"] = WiFi.softAPIP().toString();

    JsonObject ble = r.data["ble"].to<JsonObject>();
    ble["enabled"] = bleEnabled();
    ble["connected"] = bleConnected();
    ble["at_boot"] = bleAtBoot();

    JsonObject hat = r.data["hat"].to<JsonObject>();
    JsonArray live = hat["live"].to<JsonArray>();
    HatProbeResult hp;
    uint32_t at;
    if (lastHatProbe(hp, at)) {
        if (hp.cc1101) live.add("cc1101");
        if (hp.nrf24) live.add("nrf24");
        if (hp.pn532) live.add("pn532");
        if (hp.irLikely) live.add("ir");
        if (hp.ookLikely) live.add("ook433");
        hat["probed_s_ago"] = (millis() - at) / 1000;
    } else {
        hat["probed_s_ago"] = nullptr; // never probed this boot: run hat.probe
    }
}

void cmdAppsList(Request &r) {
    JsonArray apps = r.data["apps"].to<JsonArray>();
    const std::vector<String> &hidden = bruceConfig.disabledMenus;
    for (MenuItemInterface *item : mainMenu.getItems()) {
        JsonObject a = apps.add<JsonObject>();
        String name = item->getName();
        a["name"] = name;
        a["hidden"] = std::find(hidden.begin(), hidden.end(), name) != hidden.end();
    }
}

void cmdScreenNav(Request &r) {
    String key = r.args["key"] | "";
    volatile bool *btn = nullptr;
    if (key == "next") btn = &NextPress;
    else if (key == "prev") btn = &PrevPress;
    else if (key == "sel" || key == "select") btn = &SelPress;
    else if (key == "esc") btn = &EscPress;
    if (!btn) return r.fail("bad_args", "key must be next, prev, sel or esc");
    // Same as Bruce's `nav` command: a single press, the screen stays dimmed.
    AnyKeyPress = true;
    SerialCmdPress = true;
    *btn = true;
    r.data["key"] = key;
}

void cmdCliRun(Request &r) {
    String line = r.args["line"] | "";
    line.trim();
    if (line.isEmpty()) return r.fail("bad_args", "line is required");
    if (line.startsWith("rpc")) return r.fail("bad_args", "cli.run can't run rpc");

    CaptureDevice cap;
    SerialDevice *prev = serialDevice;
    serialDevice = &cap;
    bool result = serialCli.parse(line);
    if (serialDevice == &cap) serialDevice = prev; // unless the command switched transport

    // Mirror Bruce's serial handler: anything but nav/option returns the UI to the menu.
    if (!line.startsWith("nav") && !line.startsWith("option")) backToMenu();

    r.data["output"] = cap.text;
    r.data["result"] = result;
    if (cap.truncated) r.data["truncated"] = true;
}

void cmdHatProbe(Request &r) {
    if (lightshowRunning()) return r.fail("busy", "Light Show is using the hat pins");
    HatProbeResult hp = probeHat();
    r.data["pn532"] = hp.pn532;
    JsonArray other = r.data["i2c_other"].to<JsonArray>();
    for (uint8_t a : hp.i2cOther) other.add(a);
    r.data["cc1101"] = hp.cc1101;
    r.data["cc_partnum"] = hp.ccPartnum;
    r.data["cc_version"] = hp.ccVersion;
    r.data["nrf24"] = hp.nrf24;
    r.data["nrf_status"] = hp.nrfStatus;
    r.data["nrf_setup_aw"] = hp.nrfSetupAw;
    r.data["rx_checked"] = hp.rxChecked;
    r.data["rx_transitions"] = hp.rxTransitions;
    r.data["rx_high_percent"] = hp.rxHighPercent;
    r.data["ir_likely"] = hp.irLikely;
    r.data["ook_likely"] = hp.ookLikely;
    r.data["live_count"] = hp.liveCount();
    r.data["summary"] = hp.summary();
    r.data["advice"] = hp.advice();
}

REMOTE_COMMAND("system.status", cmdSystemStatus);
REMOTE_COMMAND("apps.list", cmdAppsList);
REMOTE_COMMAND("screen.nav", cmdScreenNav);
REMOTE_COMMAND("cli.run", cmdCliRun);
REMOTE_COMMAND("hat.probe", cmdHatProbe);

// ---- BLE -------------------------------------------------------------------------

// Bruce warns the BLE API needs a lot of RAM. Below this, starting it risks
// starving whatever runs next (Wi-Fi, the web UI), so refuse.
constexpr uint32_t BLE_MIN_FREE_HEAP = 45000;

} // namespace

void add(const char *name, Handler h) {
    for (Entry &e : registry())
        if (strcmp(e.name, name) == 0) {
            e.fn = h;
            return;
        }
    registry().push_back({name, h});
}

void emit(const char *name, JsonObjectConst data) {
    JsonDocument doc;
    doc["event"] = name;
    doc["data"] = data;
    // ArduinoJson 7's serializeJson(doc, String&) overwrites the string, so build
    // the JSON on its own and prefix it afterwards.
    String json;
    serializeJson(doc, json);
    String line = "\n@evt " + json + "\n"; // leading \n: never glued onto someone else's partial line
    // Events go to USB always, and to the phone on BLE when that's on.
    writeLine(&USBserial, line);
    if (bleSerialActive() && bleConnected() > 0) writeLine(serialDevice, line);
}

bool handleLine(const String &raw) {
    String line = raw;
    line.trim();
    if (!line.startsWith("rpc") || (line.length() > 3 && line[3] != ' ')) return false;

    SerialDevice *dev = serialDevice; // the transport this line came in on (patch 0014)
    JsonDocument in;
    JsonDocument out;
    String cmd;
    bool ok = false;

    DeserializationError err = deserializeJson(in, line.c_str() + 3);
    if (err || !in.is<JsonObject>()) {
        out["id"] = nullptr;
        out["ok"] = false;
        out["error"] = "bad_json";
        out["message"] = err ? err.c_str() : "expected an object";
        cmd = "?";
    } else {
        Request r;
        r.id = in["id"] | 0;
        cmd = in["cmd"] | "";
        r.cmd = cmd;
        if (!in["args"].is<JsonObject>()) in["args"].to<JsonObject>(); // missing -> {}
        r.args = in["args"].as<JsonObjectConst>();

        out["id"] = r.id;
        out["ok"] = true;
        r.data = out["data"].to<JsonObject>();

        Handler fn = cmd.isEmpty() ? nullptr : find(cmd);
        if (fn) fn(r);
        else if (planned(cmd)) r.fail("not_available", cmd + " isn't in this build yet");
        else r.fail("unknown_command", cmd.isEmpty() ? String("cmd is required") : cmd);

        ok = !r.failed();
        if (!ok) {
            out.remove("data");
            out["ok"] = false;
            out["error"] = r.error;
            out["message"] = r.message;
        }
    }

    String json;
    serializeJson(out, json); // overwrites, never appends (ArduinoJson 7): see emit()
    String reply = "\n@rpc " + json + "\n"; // leading \n: see emit()
    // A command that switched Bluetooth off (cli.run "remote ble off") has no link
    // left to answer on.
    if (dev == &USBserial || bleEnabled()) writeLine(dev, reply);
    logCommand(cmd, ok, dev);
    return true;
}

bool bleEnabled() { return isBLEAPIEnabled(); }

bool bleSetEnabled(bool on) {
    if (on == bleEnabled()) return true;
    if (on && ESP.getFreeHeap() < BLE_MIN_FREE_HEAP) {
        Serial.printf("[remote] BLE refused: %lu bytes free\n", (unsigned long)ESP.getFreeHeap());
        return false;
    }
    uint32_t before = ESP.getFreeHeap();
    enableBLEAPI(); // toggles; swaps serialDevice between USB and BLE
    Serial.printf(
        "[remote] BLE %s: heap %lu -> %lu bytes\n",
        on ? "on" : "off",
        (unsigned long)before,
        (unsigned long)ESP.getFreeHeap()
    );
    return true;
}

int bleConnected() {
    if (!bleEnabled() || !NimBLEDevice::isInitialized()) return 0;
    NimBLEServer *srv = NimBLEDevice::getServer();
    return srv ? srv->getConnectedCount() : 0;
}

bool bleAtBoot() {
    Preferences p;
    p.begin("remote", true);
    bool on = p.getBool("bleboot", false);
    p.end();
    return on;
}

void setBleAtBoot(bool on) {
    Preferences p;
    p.begin("remote", false);
    p.putBool("bleboot", on);
    p.end();
}

void boot() {
    if (bleAtBoot()) bleSetEnabled(true);
}

int recent(LogEntry *out, int maxCount) {
    portENTER_CRITICAL(&logMux);
    int n = logCount < maxCount ? logCount : maxCount;
    for (int i = 0; i < n; i++) out[i] = logRing[(logHead - 1 - i + LOG_SIZE) % LOG_SIZE];
    portEXIT_CRITICAL(&logMux);
    return n;
}

uint32_t commandCount() { return total; }

} // namespace remote
