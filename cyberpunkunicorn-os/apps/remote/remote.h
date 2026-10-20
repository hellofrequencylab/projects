// Remote: the phone-control protocol (Frequency Experience app). One JSON command
// per line over every CLI transport (USB serial, BLE serial, web /cm):
//
//   phone  -> rpc {"id":1,"cmd":"system.status","args":{}}
//   device -> @rpc {"id":1,"ok":true,"data":{...}}
//
// Full contract: apps/remote/PROTOCOL.md. Keep the two in step.
//
// Adding a command is one line in your app's .cpp:
//
//   #include "../remote/remote.h"
//   REMOTE_COMMAND("led.set", [](remote::Request &r) {
//       int red = r.args["r"] | 0;              // ArduinoJson 7: `| default`
//       if (red < 0 || red > 255) return r.fail("bad_args", "r is 0-255");
//       ledSet(red, ...);
//       r.data["r"] = red;                       // reply data (an object)
//   });
//
// Handlers run on the CLI task (8 KB stack), not the UI task: don't draw on the
// screen from one, hand work to your app's loop through a flag instead (see the
// lightshow.* handlers in apps/lightshow/LightShowMenu.cpp).
#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

namespace remote {

struct Request {
    int id = 0;
    String cmd;
    JsonObjectConst args; // always an object (empty if the phone sent none)
    JsonObject data;      // fill this; sent back as "data"

    // Turns the reply into {"ok":false,"error":code,"message":message}.
    void fail(const char *code, const String &message = "") {
        error = code;
        this->message = message;
    }
    bool failed() const { return error != nullptr; }

    const char *error = nullptr;
    String message;
};

using Handler = void (*)(Request &r);

// Registers a handler. A later registration of the same name wins.
void add(const char *name, Handler h);

// Unsolicited event: @evt {"event":name,"data":{...}} on the active transport.
// Safe from any task; lines are never interleaved with replies.
void emit(const char *name, JsonObjectConst data);

// --- used by the Remote app and hooks.cpp --------------------------------------

// Handles one raw CLI line. Returns false if it isn't an `rpc` line.
bool handleLine(const String &line);

// Bluetooth remote (Bruce's BLE API): on/off now, and the persisted boot setting.
bool bleEnabled();
bool bleSetEnabled(bool on); // false if refused (e.g. not enough free heap)
int bleConnected();          // connected phones
bool bleAtBoot();
void setBleAtBoot(bool on);
void boot(); // called once at the end of setup()

struct LogEntry {
    uint32_t ms;
    char transport[4]; // "usb" / "ble"
    char cmd[24];
    bool ok;
};
// Most recent first; returns how many were copied (max 8 kept).
int recent(LogEntry *out, int maxCount);
uint32_t commandCount();

struct Registrar {
    Registrar(const char *name, Handler h) { add(name, h); }
};

} // namespace remote

#define REMOTE_CAT2(a, b) a##b
#define REMOTE_CAT(a, b) REMOTE_CAT2(a, b)
// File-scope one-liner: REMOTE_COMMAND("name", handler);
#define REMOTE_COMMAND(name, fn) static remote::Registrar REMOTE_CAT(_remoteCmd_, __COUNTER__)(name, fn)
