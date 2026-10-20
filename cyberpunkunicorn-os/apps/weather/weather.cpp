#include "weather.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <globals.h>
#include <stdarg.h>
#include <math.h>

namespace weather {

void Settings::load() {
    Preferences p;
    p.begin("weather", true);
    metric = p.getBool("metric", metric);
    manual = p.getBool("manual", manual);
    lat = p.getFloat("lat", lat);
    lon = p.getFloat("lon", lon);
    city = p.getString("city", city);
    p.end();
}

void Settings::save() const {
    Preferences p;
    p.begin("weather", false);
    p.putBool("metric", metric);
    p.putBool("manual", manual);
    p.putFloat("lat", lat);
    p.putFloat("lon", lon);
    p.putString("city", city);
    p.end();
}

__attribute__((weak)) bool indoorClimate(float &celsius, float &humidity) { return false; }

namespace {

// GET into `body`. HTTPS without certificate checks: these are public, keyless
// lookups, and the CYD has no CA bundle to spare flash for.
bool httpGet(const String &url, String &body, String &error) {
    WiFiClientSecure tls;
    WiFiClient plain;
    HTTPClient http;
    http.setTimeout(8000);
    http.setConnectTimeout(8000);
    bool ok;
    if (url.startsWith("https://")) {
        tls.setInsecure();
        ok = http.begin(tls, url);
    } else {
        ok = http.begin(plain, url);
    }
    if (!ok) {
        error = "bad URL";
        return false;
    }
    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        error = code < 0 ? HTTPClient::errorToString(code) : "HTTP " + String(code);
        http.end();
        return false;
    }
    body = http.getString();
    http.end();
    return true;
}

// Location from the public IP. ipwho.is is free over HTTPS with no key;
// ip-api.com (HTTP only on its free tier) is the fallback.
bool locate(Data &d, String &error) {
    String body;
    JsonDocument doc;
    if (httpGet("https://ipwho.is/?fields=success,city,latitude,longitude", body, error) &&
        !deserializeJson(doc, body) && doc["success"].as<bool>()) {
        d.lat = doc["latitude"];
        d.lon = doc["longitude"];
        d.city = doc["city"].as<String>();
        return true;
    }
    if (httpGet("http://ip-api.com/json/?fields=status,city,lat,lon", body, error) &&
        !deserializeJson(doc, body) && doc["status"] == "success") {
        d.lat = doc["lat"];
        d.lon = doc["lon"];
        d.city = doc["city"].as<String>();
        return true;
    }
    if (error.isEmpty()) error = "location lookup failed";
    return false;
}

// Sakamoto's day-of-week, 0 = Sunday.
int dayOfWeek(int y, int m, int d) {
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3) y -= 1;
    return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}

} // namespace

bool fetchWeather(Data &out) {
    Data d;
    Settings s;
    s.load();
    d.metric = s.metric;
    if (WiFi.status() != WL_CONNECTED) {
        out.ok = false;
        out.error = "Wi-Fi not connected";
        return false;
    }

    String error;
    if (s.manual) {
        d.lat = s.lat;
        d.lon = s.lon;
        d.city = s.city;
    } else if (!locate(d, error)) {
        out.ok = false;
        out.error = "Location: " + error;
        return false;
    }
    if (d.city.isEmpty() || d.city == "null")
        d.city = String(d.lat, 2) + ", " + String(d.lon, 2);

    String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(d.lat, 4) +
                 "&longitude=" + String(d.lon, 4) +
                 "&current=temperature_2m,relative_humidity_2m,apparent_temperature,is_day,"
                 "weather_code,wind_speed_10m,pressure_msl"
                 "&daily=weather_code,temperature_2m_max,temperature_2m_min"
                 "&timezone=auto&forecast_days=" +
                 String(DAYS);
    if (!s.metric) url += "&temperature_unit=fahrenheit&wind_speed_unit=mph";
    String body;
    if (!httpGet(url, body, error)) {
        out.ok = false;
        out.error = "Open-Meteo: " + error;
        return false;
    }
    JsonDocument doc;
    DeserializationError je = deserializeJson(doc, body);
    if (je || !doc["current"].is<JsonObject>()) {
        out.ok = false;
        out.error = je ? String("bad JSON: ") + je.c_str() : String("no current conditions");
        return false;
    }

    JsonObject cur = doc["current"];
    d.temp = cur["temperature_2m"];
    d.feels = cur["apparent_temperature"];
    d.humidity = cur["relative_humidity_2m"];
    d.wind = cur["wind_speed_10m"];
    float hpa = cur["pressure_msl"];
    d.pressure = s.metric ? hpa : hpa * 0.02953f; // inHg, as the original showed
    d.code = cur["weather_code"];
    d.isDay = cur["is_day"].as<int>() != 0;
    String t = cur["time"].as<String>(); // "2026-10-19T14:15"
    if (t.length() >= 16) d.localTime = t.substring(11, 16);

    JsonObject daily = doc["daily"];
    for (int i = 0; i < DAYS; i++) {
        d.dayCode[i] = daily["weather_code"][i];
        d.hi[i] = daily["temperature_2m_max"][i];
        d.lo[i] = daily["temperature_2m_min"][i];
        String date = daily["time"][i].as<String>(); // "2026-10-19"
        if (date.length() >= 10)
            d.weekday[i] = dayOfWeek(date.substring(0, 4).toInt(), date.substring(5, 7).toInt(),
                                     date.substring(8, 10).toInt());
    }
    d.ok = true;
    d.fetchedAt = millis();
    out = d;
    return true;
}

const char *describe(int c) {
    switch (c) {
        case 0: return "Clear";
        case 1: return "Mostly clear";
        case 2: return "Partly cloudy";
        case 3: return "Overcast";
        case 45:
        case 48: return "Fog";
        case 51:
        case 53:
        case 55: return "Drizzle";
        case 56:
        case 57: return "Freezing drizzle";
        case 61:
        case 63: return "Rain";
        case 65: return "Heavy rain";
        case 66:
        case 67: return "Freezing rain";
        case 71:
        case 73:
        case 75:
        case 77: return "Snow";
        case 80:
        case 81:
        case 82: return "Showers";
        case 85:
        case 86: return "Snow showers";
        case 95: return "Thunderstorm";
        case 96:
        case 99: return "Thunder + hail";
        default: return "Unknown";
    }
}

int tempHue(float t, bool metric) {
    float c = metric ? t : (t - 32) * 5 / 9;
    c = constrain(c, -10.0f, 38.0f);
    return (int)(270 - (c + 10) * 270 / 48); // -10 C violet ... 38 C red
}

} // namespace weather

using namespace weather;

// Bruce 1.16.1's serialDevice->printf mangles its arguments (CLAUDE.md, gotcha 9):
// format here, then print the finished string.
static void sayf(const char *fmt, ...) {
    char buf[192];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    serialDevice->print(buf);
}

static void printData(const Data &d) {
    const char *deg = d.metric ? "C" : "F";
    sayf("Weather for %s (%.4f, %.4f), local %s\n", d.city.c_str(), d.lat, d.lon,
                         d.localTime.c_str());
    sayf("  %s, %.1f %s (feels %.1f), humidity %.0f%%\n", describe(d.code), d.temp, deg,
                         d.feels, d.humidity);
    sayf("  wind %.1f %s, pressure %.*f %s, %s\n", d.wind, d.metric ? "km/h" : "mph",
                         d.metric ? 0 : 2, d.pressure, d.metric ? "hPa" : "inHg", d.isDay ? "day" : "night");
    static const char *names[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    for (int i = 0; i < DAYS; i++)
        sayf("  %s: %s, %.0f / %.0f %s\n", names[d.weekday[i] % 7], describe(d.dayCode[i]),
                             d.hi[i], d.lo[i], deg);
    float c, h;
    if (indoorClimate(c, h)) sayf("  indoor: %.1f C, humidity %.0f%%\n", c, h);
    else serialDevice->println("  indoor: no sensor hooked up");
}

static uint32_t weatherCallback(cmd *c) {
    Command cmd(c);
    Settings s;
    s.load();
    String a0 = cmd.countArgs() > 0 ? cmd.getArgument(0).getValue() : "";
    a0.trim();
    if (a0 == "units") {
        String u = cmd.countArgs() > 1 ? cmd.getArgument(1).getValue() : "";
        if (u == "metric" || u == "imperial") {
            s.metric = (u == "metric");
            s.save();
        }
        sayf("weather units: %s\n", s.metric ? "metric" : "imperial");
        return true;
    }
    if (a0 == "loc") {
        String a1 = cmd.countArgs() > 1 ? cmd.getArgument(1).getValue() : "";
        if (a1 == "auto") {
            s.manual = false;
            s.save();
        } else if (cmd.countArgs() >= 3) {
            float lat = a1.toFloat(), lon = cmd.getArgument(2).getValue().toFloat();
            if (lat < -90 || lat > 90 || lon < -180 || lon > 180) {
                serialDevice->println("weather loc: lat must be -90..90, lon -180..180");
                return false;
            }
            s.manual = true;
            s.lat = lat;
            s.lon = lon;
            s.city = "";
            for (int i = 3; i < cmd.countArgs(); i++)
                s.city += (s.city.isEmpty() ? "" : " ") + cmd.getArgument(i).getValue();
            s.save();
        }
        if (s.manual) sayf("weather loc: %.4f, %.4f %s\n", s.lat, s.lon, s.city.c_str());
        else serialDevice->println("weather loc: auto (from public IP)");
        return true;
    }
    if (!a0.isEmpty()) {
        serialDevice->println(
            "usage: weather | weather units metric|imperial | weather loc <lat> <lon> [city] | weather loc auto"
        );
        return false;
    }
    Data d;
    serialDevice->println("Fetching weather...");
    if (!fetchWeather(d)) {
        serialDevice->println("weather: " + d.error);
        return false;
    }
    printData(d);
    return true;
}

void createWeatherCommand(SimpleCLI *cli) { cli->addBoundlessCmd("weather", weatherCallback); }
