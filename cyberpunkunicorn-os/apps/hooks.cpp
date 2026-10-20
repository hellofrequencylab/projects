#include "hooks.h"

#include "remote/remote.h"

bool cyberAppsHandleLine(const String &line, bool &result) {
    if (remote::handleLine(line)) {
        result = true;
        return true;
    }
    return false;
}

void cyberAppsBoot() { remote::boot(); }
