#include "SpectrumMenu.h"

#include "../hardware/LibraryMenu.h"
#include "../hardware/hardware.h"
#include "../remote/remote.h"
#include "../ui/cyber_icon.h"
#include "../ui/rainbow.h"
#include "core/display.h"
#include "core/scrollableTextArea.h"
#include "core/utils.h"
#include "modules/NRF24/nrf_common.h"
#include "modules/rf/rf_utils.h"
#include <WiFi.h>
#include <fft.h>
#include <globals.h>

namespace {

// ---- bands -----------------------------------------------------------------------

enum Radio : uint8_t { R_NONE, R_WIFI, R_NRF, R_CC, R_MIC };

struct Band {
    const char *id;    // remote / CLI id
    const char *label; // menu + screen
    Radio radio;
    float start, end;  // MHz (Hz for audio)
};

// CC1101 can only tune 280-350, 387-468 and 779-928 MHz: every Sub-GHz band stays inside one.
const Band BANDS[] = {
    {"wifi",    "Wi-Fi channels",          R_WIFI, 2412, 2472},
    {"2g4",     "2.4 GHz (nRF24)",         R_NRF,  2400, 2525},
    {"315",     "315 MHz (car keys, US)",  R_CC,   300,  348 },
    {"433",     "433 MHz (remotes, EU)",   R_CC,   425,  445 },
    {"868",     "868 MHz (EU devices)",    R_CC,   860,  876 },
    {"915",     "915 MHz (US devices)",    R_CC,   902,  928 },
    {"subghz_mid",  "Sub-GHz 387-464 MHz", R_CC,   387,  464 },
    {"subghz_high", "Sub-GHz 779-928 MHz", R_CC,   779,  928 },
    {"audio",   "Audio (mic, 0-4 kHz)",    R_MIC,  0,    4000},
};
constexpr int BAND_COUNT = sizeof(BANDS) / sizeof(BANDS[0]);

// ---- one sweep, shared with the CLI and remote -------------------------------------

constexpr int MAXB = 128;

struct Sweep {
    int band = -1;
    int n = 0;
    float start = 0, step = 0;   // MHz, or Hz for audio
    const char *valueUnit = "";  // what values[] holds
    uint8_t level[MAXB] = {};    // 0-100, what is drawn
    int16_t value[MAXB] = {};    // dBm / dB / % per bin
    int extra = 0;               // Wi-Fi: networks seen
    uint32_t count = 0;          // sweeps since the screen opened
};

Sweep sweep;
volatile bool screenOpen = false;
volatile bool streaming = false;

int peakIndex() {
    int best = 0;
    for (int i = 1; i < sweep.n; i++)
        if (sweep.level[i] > sweep.level[best]) best = i;
    return best;
}

String freqLabel(int band, float f) {
    char buf[24];
    if (BANDS[band].radio == R_MIC) {
        if (f >= 1000) snprintf(buf, sizeof(buf), "%.2f kHz", f / 1000);
        else snprintf(buf, sizeof(buf), "%d Hz", int(f));
    } else if (BANDS[band].radio == R_WIFI) {
        snprintf(buf, sizeof(buf), "ch %d", int((f - 2412) / 5 + 1.5f));
    } else {
        snprintf(buf, sizeof(buf), f >= 1000 ? "%.0f MHz" : "%.2f MHz", f);
    }
    return buf;
}

uint8_t toLevel(float v, float lo, float hi) {
    if (v <= lo) return 0;
    if (v >= hi) return 100;
    return uint8_t((v - lo) * 100 / (hi - lo));
}

// ---- radios ------------------------------------------------------------------------

struct Radios {
    Radio on = R_NONE;

    // Wi-Fi
    wifi_mode_t wifiWas = WIFI_OFF;
    bool scanning = false;

    // nRF24
    uint8_t nrfLevel[126] = {};

    // CC1101
    uint8_t rfModuleWas = 0;

    // Audio
    static constexpr int FFT_N = 256;
    static constexpr int SAMPLE_US = 125; // 8 kHz
    float *in = nullptr, *out = nullptr;
    fft_config_t *fft = nullptr;
    float refDb = -1000;

    bool start(Radio r);
    void stop();
    bool sweepBand(int band); // true = a fresh sweep is in `sweep`

private:
    bool wifiSweep(int band);
    bool nrfSweep(int band);
    bool ccSweep(int band);
    bool micSweep(int band);
};

Radios radios;

bool micReady() {
    if (hw::present(hw::HW_MIC)) return true;
    int pick = -1;
    std::vector<Option> opts = {
        {"How to connect it", [&pick]() { pick = 0; }},
        {"I have a mic on GPIO35", [&pick]() { pick = 1; }},
        {"Try anyway", [&pick]() { pick = 2; }},
        {"Back", [&pick]() { pick = 3; }},
    };
    for (;;) {
        pick = -1;
        loopOptions(opts, MENU_TYPE_SUBMENU, "Audio");
        if (pick == 0) {
            ScrollableTextArea area("AUDIO SPECTRUM");
            area.addLine("Needs an analog microphone board,");
            area.addLine("like a MAX4466 or MAX9814.");
            area.addLine("");
            area.addLine("Wire it to the CYD's P3 plug:");
            area.addLine("  OUT  -> GPIO35");
            area.addLine("  VCC  -> 3.3 V");
            area.addLine("  GND  -> GND");
            area.addLine("");
            area.addLine("The hat DIP switch doesn't matter.");
            area.addLine("Then pick \"I have a mic on GPIO35\".");
            area.show();
            continue;
        }
        if (pick == 1) hw::setMarked(hw::HW_MIC, true);
        return pick == 1 || pick == 2;
    }
}

bool Radios::start(Radio r) {
    if (r == on) return true;
    stop();
    switch (r) {
        case R_WIFI:
            wifiWas = WiFi.getMode();
            if (wifiWas == WIFI_OFF) WiFi.mode(WIFI_STA);
            else if (wifiWas == WIFI_AP) WiFi.mode(WIFI_AP_STA); // scanning needs the station side
            scanning = false;
            break;
        case R_NRF:
            if (!cyberPartGate("Spectrum", hw::HW_NRF24)) return false;
            if (!nrf_start(NRF_MODE_SPI)) {
                displayError("2.4 GHz radio not answering", true);
                return false;
            }
            // Bruce's nrf_spectrum setup: no ACK/CRC, short noise addresses, so RPD sees everything.
            NRFradio.setAutoAck(false);
            NRFradio.disableCRC();
            NRFradio.setAddressWidth(2);
            {
                const uint8_t noise[][2] = {
                    {0x55, 0x55}, {0xAA, 0xAA}, {0xA0, 0xAA}, {0xAB, 0xAA}, {0xAC, 0xAA}, {0xAD, 0xAA}
                };
                for (uint8_t i = 0; i < 6; ++i) NRFradio.openReadingPipe(i, noise[i]);
            }
            NRFradio.setDataRate(RF24_1MBPS);
            memset(nrfLevel, 0, sizeof(nrfLevel));
            break;
        case R_CC:
            if (!cyberPartGate("Spectrum", hw::HW_CC1101)) return false;
            rfModuleWas = bruceConfigPins.rfModule;
            bruceConfigPins.rfModule = CC1101_SPI_MODULE; // RSSI needs the CC1101, not an OOK receiver
            if (!initRfModule("rx", 433.92)) {
                bruceConfigPins.rfModule = rfModuleWas;
                return false;
            }
            break;
        case R_MIC:
            if (!micReady()) return false;
            in = (float *)malloc(FFT_N * sizeof(float));
            out = (float *)malloc(FFT_N * sizeof(float));
            fft = (in && out) ? fft_init(FFT_N, FFT_REAL, FFT_FORWARD, in, out) : nullptr;
            if (!fft) {
                free(in), free(out);
                in = out = nullptr;
                displayError("Not enough memory for audio", true);
                return false;
            }
            pinMode(35, INPUT);
            analogSetPinAttenuation(35, ADC_11db);
            refDb = -1000;
            break;
        default: return false;
    }
    on = r;
    return true;
}

void Radios::stop() {
    switch (on) {
        case R_WIFI:
            WiFi.scanDelete();
            if (wifiWas == WIFI_OFF || wifiWas == WIFI_AP) WiFi.mode(wifiWas);
            break;
        case R_NRF:
            NRFradio.stopListening();
            NRFradio.powerDown();
            break;
        case R_CC:
            deinitRfModule();
            bruceConfigPins.rfModule = rfModuleWas;
            break;
        case R_MIC:
            fft_destroy(fft);
            free(in), free(out);
            fft = nullptr, in = out = nullptr;
            break;
        default: break;
    }
    on = R_NONE;
}

bool Radios::sweepBand(int band) {
    switch (BANDS[band].radio) {
        case R_WIFI: return wifiSweep(band);
        case R_NRF: return nrfSweep(band);
        case R_CC: return ccSweep(band);
        case R_MIC: return micSweep(band);
        default: return false;
    }
}

// Async scan so the buttons stay live; each network spreads over its 22 MHz (+-2 channels).
bool Radios::wifiSweep(int band) {
    if (!scanning) {
        WiFi.scanNetworks(true, true);
        scanning = true;
        return false;
    }
    int found = WiFi.scanComplete();
    if (found == WIFI_SCAN_RUNNING) return false;
    scanning = false;
    if (found < 0) return false; // failed: next call starts another

    sweep.n = 13;
    sweep.start = 2412;
    sweep.step = 5;
    sweep.valueUnit = "dBm";
    for (int c = 0; c < 13; c++) sweep.value[c] = -100;
    for (int i = 0; i < found; i++) {
        int ch = WiFi.channel(i), rssi = WiFi.RSSI(i);
        for (int c = ch - 2; c <= ch + 2; c++) {
            if (c < 1 || c > 13) continue;
            int v = rssi - 8 * abs(c - ch);
            if (v > sweep.value[c - 1]) sweep.value[c - 1] = v;
        }
    }
    for (int c = 0; c < 13; c++) sweep.level[c] = toLevel(sweep.value[c], -95, -30);
    sweep.extra = found;
    WiFi.scanDelete();
    return true;
}

bool Radios::nrfSweep(int band) {
    for (int ch = 0; ch < 126; ch++) {
        NRFradio.setChannel(ch);
        NRFradio.startListening();
        delayMicroseconds(128);
        NRFradio.stopListening();
        int rpd = NRFradio.testRPD() ? 1 : 0;
        nrfLevel[ch] = (nrfLevel[ch] * 3 + rpd * 125) / 4;
    }
    // 126 channels into MAXB bins: plenty of room, one bin each.
    sweep.n = 126;
    sweep.start = 2400;
    sweep.step = 1;
    sweep.valueUnit = "%";
    for (int ch = 0; ch < 126; ch++) {
        sweep.level[ch] = min(100, nrfLevel[ch] * 100 / 125);
        sweep.value[ch] = sweep.level[ch];
    }
    return true;
}

bool Radios::ccSweep(int band) {
    const int n = 64;
    const Band &b = BANDS[band];
    float step = (b.end - b.start) / (n - 1);
    for (int i = 0; i < n; i++) {
        setMHZ(b.start + i * step);
        delay(5); // settle, as Bruce's RF spectrum does
        int rssi = ELECHOUSE_cc1101.getRssi();
        tft.drawPixel(0, 0, 0); // CC1101 shares the bus with the screen
        sweep.value[i] = rssi;
        sweep.level[i] = toLevel(rssi, -110, -20);
    }
    sweep.n = n;
    sweep.start = b.start;
    sweep.step = step;
    sweep.valueUnit = "dBm";
    return true;
}

bool Radios::micSweep(int band) {
    uint32_t t0 = micros();
    float mean = 0;
    for (int i = 0; i < FFT_N; i++) {
        while (micros() - t0 < uint32_t(i * SAMPLE_US)) {}
        in[i] = analogRead(35);
        mean += in[i];
    }
    mean /= FFT_N;
    for (int i = 0; i < FFT_N; i++) { // remove DC, Hann window
        float w = 0.5f - 0.5f * cosf(2 * PI * i / (FFT_N - 1));
        in[i] = (in[i] - mean) * w;
    }
    fft_execute(fft);

    // out[2k], out[2k+1] = bin k (k = 1..N/2-1); bin 0 is DC and stays empty.
    const int n = FFT_N / 2;
    float db[n];
    float top = -1000;
    db[0] = -1000;
    for (int k = 1; k < n; k++) {
        float re = out[2 * k], im = out[2 * k + 1];
        db[k] = 10 * log10f(re * re + im * im + 1e-6f);
        top = max(top, db[k]);
    }
    // Auto gain: the scale follows the loudest bin up fast and drifts back down slowly.
    refDb = max(top, refDb - 0.5f);
    for (int k = 0; k < n; k++) {
        sweep.value[k] = k ? int16_t(lroundf(db[k] - refDb)) : -60;
        sweep.level[k] = toLevel(db[k], refDb - 60, refDb);
    }
    sweep.n = n;
    sweep.start = 0;
    sweep.step = 1000000.0f / SAMPLE_US / FFT_N; // 31.25 Hz
    sweep.valueUnit = "dB";
    return true;
}

// ---- remote ------------------------------------------------------------------------

void fillSweep(JsonObject o) {
    o["band"] = BANDS[sweep.band].id;
    o["label"] = BANDS[sweep.band].label;
    o["start"] = sweep.start;
    o["step"] = sweep.step;
    o["freq_unit"] = BANDS[sweep.band].radio == R_MIC ? "Hz" : "MHz";
    o["value_unit"] = sweep.valueUnit;
    JsonArray lv = o["levels"].to<JsonArray>();
    JsonArray vals = o["values"].to<JsonArray>();
    for (int i = 0; i < sweep.n; i++) lv.add(sweep.level[i]), vals.add(sweep.value[i]);
    int p = peakIndex();
    o["peak_freq"] = sweep.start + p * sweep.step;
    o["peak_value"] = sweep.value[p];
    if (BANDS[sweep.band].radio == R_WIFI) o["networks"] = sweep.extra;
}

void emitSweep() {
    static uint32_t last = 0;
    if (!streaming || millis() - last < 250) return;
    last = millis();
    JsonDocument doc;
    fillSweep(doc.to<JsonObject>());
    remote::emit("spectrum", doc.as<JsonObjectConst>());
}

void cmdStream(remote::Request &r) {
    streaming = r.args["on"] | true;
    r.data["streaming"] = bool(streaming);
    r.data["open"] = bool(screenOpen);
}

void cmdRead(remote::Request &r) {
    if (!screenOpen || sweep.band < 0 || !sweep.count) {
        r.fail("not_open", "Open Spectrum on the device first");
        return;
    }
    fillSweep(r.data);
}

void cmdBands(remote::Request &r) {
    JsonArray a = r.data["bands"].to<JsonArray>();
    for (const Band &b : BANDS) {
        JsonObject o = a.add<JsonObject>();
        o["id"] = b.id;
        o["label"] = b.label;
    }
}

REMOTE_COMMAND("spectrum.bands", cmdBands);
REMOTE_COMMAND("spectrum.read", cmdRead);
REMOTE_COMMAND("spectrum.stream", cmdStream);

// ---- screen ------------------------------------------------------------------------

constexpr int X = 12, W = 296;
constexpr int TY = 52, TH = 92;   // trace
constexpr int WY = 150, WH = 54;  // waterfall
constexpr int AXIS_Y = WY + WH + 3;

uint16_t traceColor(int level) { return cyberHue(270 - level * 330 / 100); }

uint16_t heatColor(int level) {
    if (level < 6) return TFT_BLACK;
    if (level > 92) return TFT_WHITE;
    return cyberHue(240 - level * 300 / 100);
}

void drawFrame(int band) {
    drawMainBorderWithTitle("SPECTRUM");
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
    tft.drawString(String(BANDS[band].label) + "                ", X, 30, 1);
    cyberRainbowFrame(X - 2, TY - 2, W + 4, TH + 4, 0);
    cyberRainbowFrame(X - 2, WY - 2, W + 4, WH + 4, 180);
    tft.fillRect(X, TY, W, TH, bruceConfig.bgColor);
    tft.fillRect(X, WY, W, WH, TFT_BLACK);

    // Axis: start, middle, end.
    const Band &b = BANDS[band];
    tft.fillRect(X, AXIS_Y, W, 10, bruceConfig.bgColor);
    tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
    tft.drawString(freqLabel(band, b.start), X, AXIS_Y, 1);
    tft.drawCentreString(freqLabel(band, (b.start + b.end) / 2), X + W / 2, AXIS_Y, 1);
    tft.drawRightString(freqLabel(band, b.end), X + W, AXIS_Y, 1);
    tft.drawString("NEXT/PREV band  SEL clear peaks  ESC back", X, AXIS_Y + 12, 1);
}

void drawMessage(const String &msg) {
    tft.fillRect(X, TY, W, TH, bruceConfig.bgColor);
    tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
    tft.drawCentreString(msg, X + W / 2, TY + TH / 2 - 4, 1);
}

void drawSweep(int band, uint8_t *peak, int &wiper) {
    const int n = sweep.n;
    for (int b = 0; b < n; b++) {
        int x0 = X + b * W / n, x1 = X + (b + 1) * W / n;
        int bw = max(1, x1 - x0 - (n <= 64 ? 1 : 0)); // a gap between wide bars
        int h = sweep.level[b] * TH / 100;
        if (sweep.level[b] >= peak[b]) peak[b] = sweep.level[b];
        else if (peak[b]) peak[b]--;
        int ph = peak[b] * TH / 100;
        tft.fillRect(x0, TY, bw, TH - h, bruceConfig.bgColor);
        if (h) tft.fillRect(x0, TY + TH - h, bw, h, traceColor(sweep.level[b]));
        if (ph > h) tft.drawFastHLine(x0, TY + TH - ph, bw, TFT_WHITE);
        tft.drawFastHLine(x0, WY + wiper, x1 - x0, heatColor(sweep.level[b]));
    }
    wiper = (wiper + 1) % WH;
    tft.drawFastHLine(X, WY + wiper, W, TFT_WHITE); // the wiper: newest row is just above it

    // Readout: loudest bin now.
    int p = peakIndex();
    String s = freqLabel(band, sweep.start + p * sweep.step) + "  " + String(sweep.value[p]) + " " + sweep.valueUnit;
    if (BANDS[band].radio == R_WIFI) s += "  " + String(sweep.extra) + " networks";
    tft.setTextColor(traceColor(sweep.level[p]), bruceConfig.bgColor);
    tft.drawString(s + "          ", X, 40, 1);
}

void run(int band) {
    uint8_t peak[MAXB];
    int wiper = 0;
    bool ready = false;
    auto open = [&](int b) {
        band = b;
        memset(peak, 0, sizeof(peak));
        wiper = 0;
        sweep = Sweep();
        sweep.band = band;
        ready = radios.start(BANDS[band].radio);
        drawFrame(band);
        if (!ready) drawMessage("Not available. NEXT for another band.");
        else if (BANDS[band].radio == R_WIFI) drawMessage("Scanning Wi-Fi...");
        if (ready && BANDS[band].radio == R_CC) setMHZ(BANDS[band].start);
    };
    screenOpen = true;
    open(band);
    while (true) {
        if (ready && radios.sweepBand(band)) {
            sweep.count++;
            drawSweep(band, peak, wiper);
            emitSweep();
        }
        if (check(EscPress)) break;
        if (check(NextPress)) open((band + 1) % BAND_COUNT);
        if (check(PrevPress)) open((band + BAND_COUNT - 1) % BAND_COUNT);
        if (check(SelPress)) memset(peak, 0, sizeof(peak));
        if (!ready || BANDS[band].radio == R_WIFI) delay(10);
    }
    radios.stop();
    screenOpen = false;
}

} // namespace

void SpectrumMenu::optionsMenu() {
    int pick = -1;
    options.clear();
    for (int i = 0; i < BAND_COUNT; i++) options.push_back({BANDS[i].label, [&pick, i]() { pick = i; }});
    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_SUBMENU, "Spectrum");
    if (pick >= 0) run(pick);
}

void SpectrumMenu::drawIcon(float scale) {
    clearIconArea();
    cyberIcon(getName().c_str(), iconCenterX, iconCenterY, max(2, (int)(scale * 2.4f + 0.5f)), true);
}

// ---- CLI ---------------------------------------------------------------------------

static uint32_t spectrumCallback(cmd *c) {
    if (!screenOpen || sweep.band < 0 || !sweep.count) {
        serialDevice->println("Open Spectrum on the device first.");
        return false;
    }
    int p = peakIndex();
    serialDevice->println(String(BANDS[sweep.band].label) + ": loudest " +
                          freqLabel(sweep.band, sweep.start + p * sweep.step) + " " + String(sweep.value[p]) + " " +
                          sweep.valueUnit);
    // One character per bin, quiet to loud.
    const char *ramp = " .:-=+*#%@";
    String line;
    for (int i = 0; i < sweep.n; i++) line += ramp[min(9, sweep.level[i] / 10)];
    serialDevice->println("[" + line + "]");
    return true;
}

void createSpectrumCommand(SimpleCLI *cli) { cli->addCommand("spectrum", spectrumCallback); }
