// Frequency tag I/O on the hat's PN532 (I2C, DIP 3), through Bruce's PN532
// driver (src/modules/rfid/PN532.*): read() for detection + memory dump,
// write_ndef() with rawNdefRecord for the URI record. Only the NTAG21x lock
// bytes, which Bruce has no API for, are written here with the driver's own
// Adafruit_PN532 instance.
#pragma once

#include <Arduino.h>
#include <functional>

namespace freq {

enum CardStatus {
    CARD_OK,
    CARD_NONE,          // no tag in the field (yet)
    CARD_NO_READER,     // PN532 not answering: DIP 3 off
    CARD_BUSY,          // the other side (screen or serial) has the reader
    CARD_NOT_NTAG,      // MIFARE Classic, Type 4, plain Ultralight...
    CARD_TOO_LONG,      // URL doesn't fit this tag's memory
    CARD_LOCKED,        // already write-locked
    CARD_WRITE_FAILED,
    CARD_VERIFY_FAILED, // wrote, but read back something else
};
const char *cardStatusText(CardStatus s);

struct CardInfo {
    String uid;  // hex, no spaces
    String type; // "NTAG213", "MIFARE Classic 1K", ...
    bool ntag = false;
    bool locked = false; // static lock bytes set: user pages are read-only
    int capacity = 0;    // NDEF bytes available (NTAG only)
    String uri;          // first NDEF URI record, "" if none
};

// Claim the reader for one screen or one CLI command: probes the hat
// (probeHat) and starts the PN532. CARD_NO_READER means "turn DIP 3 ON".
CardStatus readerOpen();
void readerClose();

// One read attempt. CARD_NONE when nothing is in the field.
CardStatus pollCard(CardInfo &out);

// Poll until a tag answers, `timeoutMs` passes (0 = forever) or cancel()
// returns true. `idle` runs between polls (animation).
CardStatus waitCard(
    CardInfo &out, uint32_t timeoutMs, const std::function<bool()> &cancel,
    const std::function<void()> &idle = nullptr
);

// How the reader's selects ended since resetPollStats(), so a timeout can say
// whether the PN532 never answered (noAck / noAnswer: the reader or its bus)
// or answered "no card" every time (noCard: nothing in its field).
struct PollStats {
    uint32_t polls = 0, noAck = 0, noAnswer = 0, noCard = 0, selected = 0;
};
void resetPollStats();
PollStats pollStats();

// Write `uri` as the tag's only NDEF record, then read it back. `out` is the
// verified tag.
CardStatus writeCard(const String &uri, CardInfo &out);

// Set the NTAG21x static + dynamic lock bits on the tag in the field.
// PERMANENT: the user pages can never be written again.
CardStatus lockCard(const String &expectUid);

} // namespace freq
