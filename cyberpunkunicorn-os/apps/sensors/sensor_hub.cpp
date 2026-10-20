#include "sensor_hub.h"

#include "../hat/hat_probe.h"
#include "core/bus_HAL.h"
#include "core/sd_functions.h"
#include "sensor_packet.h"
#include <LittleFS.h>
#include <Preferences.h>
#include <SD.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <globals.h>
#include <time.h>

namespace sensors {

namespace {

// ---- state (guarded by the hub lock) -------------------------------------------

SemaphoreHandle_t hubMutex() {
    static SemaphoreHandle_t m = xSemaphoreCreateRecursiveMutex();
    return m;
}

struct Lock {
    Lock() { xSemaphoreTakeRecursive(hubMutex(), portMAX_DELAY); }
    ~Lock() { xSemaphoreGiveRecursive(hubMutex()); }
};

Channel channels[MAX_CHANNELS];
int nChannels = 0;
WiredDevice wired[MAX_WIRED];
int nWired = 0;
TwoWire *bus = nullptr;
int sessionDepth = 0;
SessionInfo info;
float altBaseline = 0;

// ---- ESP-NOW receive queue (filled from the Wi-Fi task) ------------------------

struct RxItem {
    cu_sensor_packet_t p;
    int8_t rssi;
};
constexpr int RXQ = 16;
RxItem rxQueue[RXQ];
volatile uint8_t rxHead = 0, rxTail = 0;
volatile uint32_t rxIgnored = 0, rxDropped = 0, rxTotal = 0;
portMUX_TYPE rxMux = portMUX_INITIALIZER_UNLOCKED;
bool wirelessOn = false;
bool weStartedWifi = false;

void onEspNowRecv(const esp_now_recv_info_t *ri, const uint8_t *data, int len) {
    if (!cu_sensor_packet_valid(data, len)) {
        rxIgnored = rxIgnored + 1;
        return;
    }
    RxItem it;
    memcpy(&it.p, data, sizeof(it.p));
    it.rssi = (ri && ri->rx_ctrl) ? ri->rx_ctrl->rssi : 0;
    portENTER_CRITICAL(&rxMux);
    uint8_t next = (rxHead + 1) % RXQ;
    if (next != rxTail) {
        rxQueue[rxHead] = it;
        rxHead = next;
        rxTotal = rxTotal + 1;
    } else rxDropped = rxDropped + 1;
    portEXIT_CRITICAL(&rxMux);
}

bool startWireless(int &channelOut) {
    weStartedWifi = WiFi.getMode() == WIFI_MODE_NULL;
    if (weStartedWifi) {
        WiFi.mode(WIFI_STA);
        esp_wifi_set_channel(CU_SENSOR_CHANNEL, WIFI_SECOND_CHAN_NONE);
    }
    if (esp_now_init() != ESP_OK) {
        if (weStartedWifi) WiFi.mode(WIFI_OFF);
        weStartedWifi = false;
        return false;
    }
    esp_now_register_recv_cb(onEspNowRecv);
    uint8_t primary = 0;
    wifi_second_chan_t second;
    esp_wifi_get_channel(&primary, &second);
    channelOut = primary;
    wirelessOn = true;
    return true;
}

// Bruce's file sharing (core/connect) also inits ESP-NOW and registers its own
// receive callback, but only while its screen is open; sessions here are modal
// too, so the two never overlap. Tear down fully so it starts clean.
void stopWireless() {
    if (!wirelessOn) return;
    esp_now_unregister_recv_cb();
    esp_now_deinit();
    if (weStartedWifi) WiFi.mode(WIFI_OFF);
    weStartedWifi = false;
    wirelessOn = false;
}

// ---- channels ------------------------------------------------------------------

Channel *findOrAdd(const char *source, const char *label, uint8_t type, uint8_t unit, bool isWireless,
                   uint32_t nodeId) {
    for (int i = 0; i < nChannels; i++) {
        Channel &c = channels[i];
        if (c.wireless == isWireless && c.nodeId == nodeId && c.type == type && !strcmp(c.source, source) &&
            !strcmp(c.label, label))
            return &c;
    }
    if (nChannels >= MAX_CHANNELS) return nullptr;
    Channel &c = channels[nChannels++];
    memset(&c, 0, sizeof(c));
    strlcpy(c.source, source, sizeof(c.source));
    strlcpy(c.label, label, sizeof(c.label));
    c.type = type;
    c.unit = unit;
    c.wireless = isWireless;
    c.nodeId = nodeId;
    c.battery = CU_SENSOR_BATTERY_NONE;
    return &c;
}

void dropWiredChannels() {
    int j = 0;
    for (int i = 0; i < nChannels; i++)
        if (channels[i].wireless) {
            if (i != j) channels[j] = channels[i];
            j++;
        }
    nChannels = j;
}

void takeWireless(const RxItem &it) {
    const cu_sensor_packet_t &p = it.p;
    char source[20];
    char name[CU_SENSOR_NAME_LEN + 1] = {};
    memcpy(name, p.name, CU_SENSOR_NAME_LEN);
    if (name[0]) strlcpy(source, name, sizeof(source));
    else snprintf(source, sizeof(source), "node %04X", (unsigned)(p.node_id & 0xFFFF));
    uint8_t type = p.type <= CU_TYPE_LAST ? p.type : CU_TYPE_GENERIC;
    uint8_t unit = p.unit <= CU_UNIT_LAST ? p.unit : CU_UNIT_NONE;
    Channel *c = findOrAdd(source, typeText(type), type, unit, true, p.node_id);
    if (!c) return;
    if (c->updatedMs) {
        uint16_t gap = (uint16_t)(p.seq - c->seq);
        if (gap > 1 && gap < 1000) c->lost += gap - 1;
    }
    c->seq = p.seq;
    c->unit = unit;
    c->battery = p.battery;
    c->rssi = it.rssi;
    c->value = p.value;
    c->updatedMs = millis();
    c->push(p.value);
}

// ---- logging -------------------------------------------------------------------

constexpr uint32_t LOG_INTERVAL_MS = 10000;
constexpr size_t LOG_CAP_LITTLEFS = 24 * 1024; // x2 with the rotated file; LittleFS is ~190 KB
constexpr size_t LOG_CAP_SD = 4 * 1024 * 1024;
constexpr size_t LITTLEFS_KEEP_FREE = 32 * 1024;
const char *SD_DIR = "/sensors";
const char *SD_LOG = "/sensors/log.csv";
const char *SD_OLD = "/sensors/log.1.csv";
const char *LFS_LOG = "/sensors.csv";
const char *LFS_OLD = "/sensors.1.csv";

int8_t logEnabled = -1; // -1 = not loaded from NVS yet
uint32_t lastLogMs = 0;
uint32_t logRows = 0;
String logError;

bool logOn() {
    if (logEnabled < 0) {
        Preferences p;
        p.begin("sensors", true);
        logEnabled = p.getBool("log", false) ? 1 : 0;
        p.end();
    }
    return logEnabled == 1;
}

void rotate(FS &fs, const char *path, const char *old, size_t cap) {
    File f = fs.open(path, FILE_READ);
    if (!f) return;
    size_t size = f.size();
    f.close();
    if (size < cap) return;
    fs.remove(old);
    fs.rename(path, old);
}

void writeLog(uint32_t now) {
    FS *fs = nullptr;
    const char *path = nullptr;
    if (sdcardMounted) {
        fs = &SD;
        path = SD_LOG;
        if (!SD.exists(SD_DIR)) SD.mkdir(SD_DIR);
        rotate(SD, SD_LOG, SD_OLD, LOG_CAP_SD);
    } else {
        rotate(LittleFS, LFS_LOG, LFS_OLD, LOG_CAP_LITTLEFS);
        if (LittleFS.totalBytes() - LittleFS.usedBytes() < LITTLEFS_KEEP_FREE) {
            logError = "LittleFS nearly full";
            return;
        }
        fs = &LittleFS;
        path = LFS_LOG;
    }
    bool fresh = !fs->exists(path);
    File f = fs->open(path, FILE_APPEND);
    if (!f) {
        logError = String("can't open ") + path;
        return;
    }
    if (fresh) f.println("epoch,uptime_s,source,quantity,value,unit,link");
    time_t t = time(nullptr);
    String epoch = t > 1700000000 ? String((uint32_t)t) : String("");
    for (int i = 0; i < nChannels; i++) {
        const Channel &c = channels[i];
        // Only readings that changed since the last row: no repeats of a dead node.
        if (!c.updatedMs || now - c.updatedMs > LOG_INTERVAL_MS) continue;
        f.printf(
            "%s,%lu,%s,%s,%s,%s,%s\n",
            epoch.c_str(),
            (unsigned long)(now / 1000),
            c.source,
            c.label,
            formatValue(c.value, c.unit).c_str(),
            unitText(c.unit),
            c.wireless ? "espnow" : "i2c"
        );
        logRows++;
    }
    f.close();
    logError = "";
}

String hex2(uint8_t v) {
    char b[5];
    snprintf(b, sizeof(b), "0x%02X", v);
    return b;
}

String ago(uint32_t ms) {
    uint32_t s = ms / 1000;
    if (s < 120) return String(s) + "s ago";
    if (s < 7200) return String(s / 60) + "m ago";
    return String(s / 3600) + "h ago";
}

} // namespace

// ---- Channel -------------------------------------------------------------------

void Channel::push(float v) {
    hist[histHead] = v;
    histHead = (histHead + 1) % HISTORY;
    if (histCount < HISTORY) histCount++;
}

float Channel::histAt(int i) const {
    int start = (histHead - histCount + HISTORY) % HISTORY;
    return hist[(start + i) % HISTORY];
}

// ---- text ----------------------------------------------------------------------

const char *unitText(uint8_t unit) {
    static const char *const U[] = {"",    "C",  "%", "hPa", "lx",  "ppm", "ppb", "V",
                                    "mA",  "mW", "g", "dps", "deg", "m",   "cm"};
    return unit <= CU_UNIT_LAST ? U[unit] : "";
}

const char *typeText(uint8_t type) {
    static const char *const T[] = {"Value",  "Temp",     "Humidity", "Pressure", "Light", "CO2",
                                    "TVOC",   "Voltage",  "Current",  "Power",    "Accel", "Gyro",
                                    "Angle",  "Altitude", "Distance", "Soil",     "Motion", "Count"};
    return type <= CU_TYPE_LAST ? T[type] : "Value";
}

String formatValue(float v, uint8_t unit) {
    if (isnan(v)) return "--";
    int decimals = 1;
    switch (unit) {
        case CU_UNIT_PPM:
        case CU_UNIT_PPB:
        case CU_UNIT_LUX:
            if (fabsf(v) >= 100) decimals = 0;
            break;
        case CU_UNIT_VOLT:
        case CU_UNIT_G: decimals = 2; break;
        case CU_UNIT_HPA: decimals = 1; break;
        default:
            if (fabsf(v) >= 1000) decimals = 0;
            break;
    }
    return String(v, decimals);
}

// ---- sessions ------------------------------------------------------------------

SessionInfo beginSession(bool wireless) {
    Lock l;
    if (sessionDepth++ > 0) {
        // Nested (CLI while the screen is open): reuse what's running.
        if (wireless && !wirelessOn) info.wirelessOk = startWireless(info.espnowChannel);
        return info;
    }
    info = SessionInfo();
    rxHead = 0;
    rxTail = 0;

    HatProbeResult hp = probeHat();
    info.hat = hp.summary();
    dropWiredChannels();
    nWired = 0;
    if (hp.cc1101 || hp.nrf24) {
        info.warning = hp.summary() + " owns pins 22/27. Turn DIP " + (hp.cc1101 ? "1" : "2") +
                       " OFF to use Grove sensors.";
    } else {
        info.wiredOk = true;
        std::vector<uint8_t> addrs = hp.i2cOther;
        if (hp.pn532) addrs.insert(addrs.begin(), 0x24);
        bus = acquireI2CBus();
        for (uint8_t a : addrs) {
            if (nWired >= MAX_WIRED) break;
            if (identify(bus, a, wired[nWired])) nWired++;
        }
        if (hp.irLikely || hp.ookLikely)
            info.warning = "IR/433 module may be on (DIP 4/5): it shares pin 27 with SDA.";
    }
    if (wireless) info.wirelessOk = startWireless(info.espnowChannel);
    if (logOn() && !sdcardMounted) setupSdCard();
    lastLogMs = millis();
    return info;
}

void endSession() {
    Lock l;
    if (sessionDepth == 0) return;
    if (--sessionDepth > 0) return;
    stopWireless();
    if (bus) {
        releaseI2CBus();
        bus = nullptr;
    }
}

bool sessionActive() {
    Lock l;
    return sessionDepth > 0;
}

void poll() {
    Lock l;
    if (sessionDepth == 0) return;
    uint32_t now = millis();

    if (bus) {
        Reading r[MAX_READINGS];
        for (int i = 0; i < nWired; i++) {
            WiredDevice &d = wired[i];
            int n = sensors::poll(bus, d, r, now);
            if (n <= 0) continue;
            char source[20];
            snprintf(source, sizeof(source), "%s %02X", d.name, d.addr);
            for (int k = 0; k < n; k++) {
                Channel *c = findOrAdd(source, r[k].label, r[k].type, r[k].unit, false, d.addr);
                if (!c) continue;
                c->value = r[k].value;
                c->updatedMs = now;
                c->push(r[k].value);
            }
        }
    }

    while (true) {
        RxItem it;
        portENTER_CRITICAL(&rxMux);
        bool have = rxTail != rxHead;
        if (have) {
            it = rxQueue[rxTail];
            rxTail = (rxTail + 1) % RXQ;
        }
        portEXIT_CRITICAL(&rxMux);
        if (!have) break;
        takeWireless(it);
    }

    if (logOn() && now - lastLogMs >= LOG_INTERVAL_MS) {
        lastLogMs = now;
        writeLog(now);
    }
}

int channelCount() {
    Lock l;
    return nChannels;
}

bool channel(int i, Channel &out) {
    Lock l;
    if (i < 0 || i >= nChannels) return false;
    out = channels[i];
    return true;
}

std::vector<WiredDevice> wiredDevices() {
    Lock l;
    return std::vector<WiredDevice>(wired, wired + nWired);
}

SessionInfo sessionInfo() {
    Lock l;
    return info;
}

std::vector<String> scanReport() {
    Lock l;
    bool own = sessionDepth == 0;
    if (own) beginSession(false);
    std::vector<String> out;
    out.push_back("Hat: " + info.hat);
    if (!info.wiredOk) out.push_back(info.warning);
    else {
        out.push_back("Grove I2C: SDA 27 / SCL 22");
        if (!info.warning.isEmpty()) out.push_back(info.warning);
        if (nWired == 0) out.push_back("Nothing answered on the bus.");
        for (int i = 0; i < nWired; i++) {
            const WiredDevice &d = wired[i];
            String s = hex2(d.addr) + "  " + d.name;
            while (s.length() < 20) s += ' ';
            if (d.chip == Chip::PN532) s += "hat NFC, shares bus";
            else if (readable(d.chip)) s += "live";
            else s += "no driver";
            out.push_back(s);
        }
    }
    if (own) endSession();
    return out;
}

std::vector<String> readingsReport() {
    Lock l;
    std::vector<String> out;
    uint32_t now = millis();
    if (!info.wiredOk && sessionDepth > 0) out.push_back("Wired: off. " + info.warning);
    else out.push_back("Wired (Grove I2C, SDA 27 / SCL 22):");
    int shown = 0;
    for (int i = 0; i < nChannels; i++) {
        const Channel &c = channels[i];
        if (c.wireless) continue;
        String s = String("  ") + c.source;
        while (s.length() < 14) s += ' ';
        s += c.label;
        while (s.length() < 24) s += ' ';
        s += formatValue(c.value, c.unit) + " " + unitText(c.unit);
        out.push_back(s);
        shown++;
    }
    for (int i = 0; i < nWired; i++)
        if (!readable(wired[i].chip))
            out.push_back("  " + hex2(wired[i].addr) + " " + wired[i].name + " (no readings)");
    if (!shown && !nWired && info.wiredOk) out.push_back("  nothing on the bus");

    if (wirelessOn) out.push_back("Wireless (ESP-NOW, channel " + String(info.espnowChannel) + "):");
    else out.push_back("Wireless (ESP-NOW receiver off; `sensors listen 10` to listen):");
    shown = 0;
    for (int i = 0; i < nChannels; i++) {
        const Channel &c = channels[i];
        if (!c.wireless) continue;
        char id[10];
        snprintf(id, sizeof(id), "%08lX", (unsigned long)c.nodeId);
        String s = String("  ") + c.source;
        while (s.length() < 14) s += ' ';
        s += c.label;
        while (s.length() < 24) s += ' ';
        s += formatValue(c.value, c.unit) + " " + unitText(c.unit);
        s += String("  id ") + id + "  rssi " + String(c.rssi);
        if (c.battery != CU_SENSOR_BATTERY_NONE) s += "  batt " + String(c.battery) + "%";
        if (c.lost) s += "  lost " + String(c.lost);
        s += "  " + ago(now - c.updatedMs);
        out.push_back(s);
        shown++;
    }
    if (!shown) out.push_back("  no nodes heard");
    if (rxIgnored || rxDropped)
        out.push_back("  ignored " + String(rxIgnored) + " non-sensor packets, dropped " + String(rxDropped));

    String lg = String("Log: ") + (logOn() ? "on -> " + logTarget() : String("off"));
    if (logOn()) lg += ", " + String(logRows) + " rows this boot";
    if (!logError.isEmpty()) lg += " (" + logError + ")";
    out.push_back(lg);
    return out;
}

bool loggingEnabled() {
    Lock l;
    return logOn();
}

void setLoggingEnabled(bool on) {
    Lock l;
    logEnabled = on ? 1 : 0;
    Preferences p;
    p.begin("sensors", false);
    p.putBool("log", on);
    p.end();
    if (on && !sdcardMounted) setupSdCard();
}

String logTarget() {
    if (sdcardMounted) return String("SD:") + SD_LOG;
    return String("LittleFS:") + LFS_LOG;
}

float altimeterBaseline() {
    Lock l;
    return altBaseline;
}

void setAltimeterBaseline(float hPa) {
    Lock l;
    altBaseline = hPa;
}

} // namespace sensors
