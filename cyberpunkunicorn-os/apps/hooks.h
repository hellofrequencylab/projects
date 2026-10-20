// Hooks Bruce calls into our apps (patch 0014). Kept separate from apps.h so the
// Bruce files that call them don't pull in every app's menu header.
#pragma once

#include <Arduino.h>

// Every CLI line (USB, BLE, web /cm, JS) passes through here before SimpleCLI.
// Return true if an app handled it; `result` is then what parse() returns.
bool cyberAppsHandleLine(const String &line, bool &result);

// End of setup(), just before the CLI task starts reading.
void cyberAppsBoot();
