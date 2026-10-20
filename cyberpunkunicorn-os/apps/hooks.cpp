#include "hooks.h"

#include "hardware/hardware.h"
#include "remote/remote.h"

bool cyberAppsHandleLine(const String &line, bool &result) {
    if (remote::handleLine(line)) {
        result = true;
        return true;
    }
    return false;
}

void cyberAppsBoot() {
    hw::boot(); // first hat probe: sorts apps into the main menu or the Library
    remote::boot();
}
