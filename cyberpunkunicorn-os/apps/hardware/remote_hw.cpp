// Remote commands for the hardware registry / Library. Contract: apps/remote/PROTOCOL.md.
#include "../remote/remote.h"
#include "core/main_menu.h"
#include "hardware.h"
#include <globals.h>

namespace {

void fillParts(JsonArray parts) {
    for (int i = 0; i < hw::HW_PART_COUNT; i++) {
        hw::Part p = hw::Part(i);
        JsonObject o = parts.add<JsonObject>();
        o["id"] = hw::info(p).id;
        o["name"] = hw::info(p).name;
        o["present"] = hw::present(p);
        o["live"] = hw::liveNow(p);
        o["detected"] = hw::detected(p);
        o["marked"] = hw::marked(p);
        o["auto"] = hw::info(p).autoDetect;
        o["how"] = hw::info(p).howTo;
    }
}

void fillApps(JsonArray apps) {
    for (MenuItemInterface *item : mainMenu.getItems()) {
        const hw::AppNeed *n = hw::needFor(item->getName());
        if (!n) continue;
        JsonObject a = apps.add<JsonObject>();
        a["name"] = item->getName();
        a["active"] = hw::appActive(item->getName());
        JsonArray needs = a["needs_any"].to<JsonArray>();
        for (int i = 0; i < hw::HW_PART_COUNT; i++)
            if (n->anyOf & hw::partBit(hw::Part(i))) needs.add(hw::info(hw::Part(i)).id);
    }
}

// hardware.list -> {parts:[...], apps:[...]} (apps = only those with a hardware rule)
void cmdList(remote::Request &r) {
    fillParts(r.data["parts"].to<JsonArray>());
    fillApps(r.data["apps"].to<JsonArray>());
}

// hardware.rescan -> same as hardware.list, after a fresh probe
void cmdRescan(remote::Request &r) {
    if (!hw::rescan()) return r.fail("busy", "Light Show is using the hat pins");
    cmdList(r);
}

// hardware.mark {part, have} -> same as hardware.list
void cmdMark(remote::Request &r) {
    String id = r.args["part"] | "";
    hw::Part p;
    if (!hw::partFromId(id, p)) return r.fail("bad_args", "unknown part");
    if (!r.args["have"].is<bool>()) return r.fail("bad_args", "have must be true or false");
    bool have = r.args["have"];
    if (have) hw::setMarked(p, true);
    else hw::forget(p);
    cmdList(r);
}

} // namespace

REMOTE_COMMAND("hardware.list", cmdList);
REMOTE_COMMAND("hardware.rescan", cmdRescan);
REMOTE_COMMAND("hardware.mark", cmdMark);
