#pragma once

#include <Arduino.h>
#include <SimpleCLI.h>

// Weather data for the Weather app: Open-Meteo forecast (free, no API key), with
// the location estimated from the public IP unless lat/lon are saved. Settings
// live in NVS namespace "weather" (metric, lat, lon, city). Nothing here touches
// the network on its own: only the app (when opened) and the `weather` command call
// fetchWeather(), and only if Wi-Fi is already connected.

namespace weather {

constexpr int DAYS = 3;

struct Settings {
    bool metric = false; // the original launcher defaulted to imperial
    bool manual = false; // true = use lat/lon below instead of IP geolocation
    float lat = 0, lon = 0;
    String city;

    void load();
    void save() const;
};

struct Data {
    bool ok = false;
    String error;
    bool metric = false;
    String city;
    float lat = 0, lon = 0;
    String localTime; // "HH:MM" at the location, from Open-Meteo
    float temp = 0, feels = 0, humidity = 0, wind = 0, pressure = 0; // pressure: hPa or inHg
    int code = 0;      // WMO weather code
    bool isDay = true;
    int dayCode[DAYS] = {};
    float hi[DAYS] = {}, lo[DAYS] = {};
    int weekday[DAYS] = {}; // 0 = Sunday
    uint32_t fetchedAt = 0; // millis()
};

// Fills `out`. Returns false with out.error set (e.g. "Wi-Fi not connected").
bool fetchWeather(Data &out);

// Short text for a WMO weather code ("Partly cloudy", "Rain", ...).
const char *describe(int code);
// Temperature -> rainbow hue (violet = cold, red = hot), in the given units.
int tempHue(float t, bool metric);

// Hook for the indoor reading. The original launcher showed an indoor temperature
// from a BMP280; sensor drivers belong to the Sensors app, so the Weather app
// doesn't carry one. To light up the "Indoor" page, define this (non-weak) in the
// Sensors app, returning true with celsius filled in (humidity = NAN if unknown).
// The weak default here returns false and the page says no sensor was found.
bool indoorClimate(float &celsius, float &humidity);

} // namespace weather

// Serial: `weather` fetches and prints; `weather units metric|imperial`;
// `weather loc <lat> <lon> [city]`; `weather loc auto`.
void createWeatherCommand(SimpleCLI *cli);
