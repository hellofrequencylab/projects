// Internet relay (plan Phase 4b): the website controls the board from any network.
// Both sides are clients of Supabase Realtime and meet on the private channel
// "device:<id>". The board mints a 15-minute token from frequency-web
// (POST /api/v1/devices/realtime-token, X-Frequency-Device), joins over a TLS
// WebSocket, and runs `rpc` lines that arrive as broadcast "rpc" through the same
// code as USB. Contract: PROTOCOL.md section 6.
//
// Off by default (NVS remote/relay). Needs Wi-Fi, a provisioned scanner
// (frequency.provision) and the RAM for TLS, so it never runs beside Bluetooth.
#pragma once

#include <Arduino.h>

namespace relay {

enum State {
    OFF,          // switched off
    WAITING,      // on, but no Wi-Fi, no scanner key, Bluetooth on or low RAM
    CONNECTING,   // minting a token or opening the socket
    JOINED,       // on the channel: the website can send commands
    REFUSED,      // the site said no (relay off for this device, or not enabled there)
};

bool enabled();
void setEnabled(bool on); // saved in NVS; starts or stops the background task
State state();
const char *stateText(State s);
String lastError(); // why the last attempt failed, for the Remote screen

// Sends an @evt line's JSON ({"event":..., "data":...}) to the website while
// joined. Safe from any task; drops it when not joined.
void sendEvent(const String &json);

// Closes the socket for a while so a check-in POST has the RAM for its own TLS
// session. Called by the Frequency backend; the relay rejoins on its own.
void yieldTls();

void boot(); // from remote::boot(): starts the task when the relay is on

} // namespace relay
