// PN532.h has no include guard and opens up Adafruit_PN532's private members
// (#define private public), so it must come before anything else that could
// pull in Adafruit_PN532.h.
#include "modules/rfid/PN532.h"

#include "../hat/hat_probe.h"
#include "card.h"
#include "flash.h"
#include <freertos/semphr.h>
#include <vector>

namespace freq {

namespace {

PN532 *rfid = nullptr;

SemaphoreHandle_t readerMutex() {
    static SemaphoreHandle_t m = xSemaphoreCreateMutex();
    return m;
}
bool held = false; // only touched by the holder

bool isNtagSize(int pages) { return pages == 45 || pages == 135 || pages == 231; }

String ntagName(int pages) {
    switch (pages) {
        case 45: return "NTAG213";
        case 135: return "NTAG215";
        case 231: return "NTAG216";
        default: return "";
    }
}

int capacityFor(int pages) {
    switch (pages) {
        case 45: return 144;
        case 135: return 504;
        case 231: return 888;
        default: return 0;
    }
}

// One short NDEF URI record, with the standard scheme abbreviation.
std::vector<uint8_t> uriRecord(const String &uri) {
    static const char *prefixes[] = {"", "http://www.", "https://www.", "http://", "https://"};
    uint8_t code = 0;
    for (uint8_t i = 1; i < 5; i++)
        if (uri.startsWith(prefixes[i])) {
            code = i;
            break;
        }
    String rest = uri.substring(strlen(prefixes[code]));
    std::vector<uint8_t> rec = {0xD1, 0x01, (uint8_t)(rest.length() + 1), 'U', code};
    for (char c : rest) rec.push_back((uint8_t)c);
    return rec;
}

// Bruce's read() leaves the card memory as text: "Page N: AA BB CC DD\n".
std::vector<uint8_t> pagesFromDump(const String &dump, int totalPages) {
    std::vector<uint8_t> bytes(totalPages * 4, 0);
    int pos = 0;
    while (pos < (int)dump.length()) {
        int end = dump.indexOf('\n', pos);
        if (end < 0) end = dump.length();
        String line = dump.substring(pos, end);
        pos = end + 1;
        if (!line.startsWith("Page ")) continue;
        int colon = line.indexOf(':');
        int page = line.substring(5, colon).toInt();
        if (colon < 0 || page < 0 || page >= totalPages) continue;
        String hex = line.substring(colon + 1);
        hex.replace(" ", "");
        for (int b = 0; b < 4 && b * 2 + 1 < (int)hex.length(); b++)
            bytes[page * 4 + b] = strtoul(hex.substring(b * 2, b * 2 + 2).c_str(), nullptr, 16);
    }
    return bytes;
}

// NFC Forum URI record prefix codes (only the ones a loyalty URL could use).
const char *uriPrefix(uint8_t code) {
    switch (code) {
        case 0x01: return "http://www.";
        case 0x02: return "https://www.";
        case 0x03: return "http://";
        case 0x04: return "https://";
        default: return "";
    }
}

// First URI record of the first NDEF message TLV in Type 2 user memory.
String parseNdefUri(const std::vector<uint8_t> &mem) {
    size_t p = 16; // page 4
    while (p + 1 < mem.size()) {
        uint8_t t = mem[p];
        if (t == 0x00) { // NULL TLV
            p++;
            continue;
        }
        if (t == 0xFE) break; // terminator
        size_t len = mem[p + 1], hdr = 2;
        if (len == 0xFF && p + 3 < mem.size()) {
            len = (mem[p + 2] << 8) | mem[p + 3];
            hdr = 4;
        }
        if (t != 0x03) { // lock / memory control / proprietary
            p += hdr + len;
            continue;
        }
        size_t r = p + hdr, end = min(r + len, mem.size());
        if (r + 3 > end) return "";
        uint8_t flags = mem[r];
        bool sr = flags & 0x10, il = flags & 0x08;
        uint8_t typeLen = mem[r + 1];
        size_t q = r + 2;
        uint32_t payLen = 0;
        if (sr) payLen = mem[q++];
        else {
            if (q + 4 > end) return "";
            payLen = (mem[q] << 24) | (mem[q + 1] << 16) | (mem[q + 2] << 8) | mem[q + 3];
            q += 4;
        }
        uint8_t idLen = il ? mem[q++] : 0;
        if ((flags & 0x07) != 0x01 || typeLen != 1 || q >= end || mem[q] != 'U') return "";
        q += typeLen + idLen;
        if (payLen < 1 || q + payLen > end) return "";
        String uri = uriPrefix(mem[q]);
        for (size_t i = 1; i < payLen; i++) uri += (char)mem[q + i];
        return uri;
    }
    return "";
}

// A reliable "is a card in the field" check. Bruce's read() reads the
// InListPassiveTarget answer without waiting for the PN532 to be ready, so on
// its own it misses cards now and then; waiting for RDY here first fixes that.
bool selectCard(uint16_t waitMs) {
    if (!rfid->nfc.startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A)) return false;
    if (!rfid->nfc.waitready(waitMs)) return false;
    if (!rfid->nfc.readDetectedPassiveTargetID()) return false;
    return true;
}

// Reads the card selectCard() just found. Bruce's read() and write_ndef() send
// InListPassiveTarget again and read the answer without waiting for RDY (over
// I2C nothing waits), so with a tag held still they could fail every retry and
// the tag was never seen. Here the select already waited; read on from it.
int readSelected() {
    rfid->format_data();
    rfid->set_uid();
    rfid->pageReadStatus = rfid->read_data_blocks();
    rfid->pageReadSuccess = rfid->pageReadStatus == RFIDInterface::SUCCESS;
    return RFIDInterface::SUCCESS;
}

// The card is selected; read it, selecting it again if a page read dropped.
// SUCCESS once any select worked, as Bruce's read() did: a tag whose pages
// can't be read (not an NTAG) is still a tag, and fill() says what it is.
int readFull() {
    bool seen = false;
    for (int i = 0; i < 3; i++) {
        if (i) {
            delay(15);
            if (!selectCard(200)) continue;
        }
        seen = true;
        readSelected();
        if (rfid->pageReadSuccess || rfid->uid.sak != PN532::PICC_TYPE_MIFARE_UL) break;
    }
    return seen ? RFIDInterface::SUCCESS : RFIDInterface::TAG_NOT_PRESENT;
}

void fill(CardInfo &out) {
    out = CardInfo();
    out.uid = rfid->printableUID.uid;
    out.uid.replace(" ", "");
    out.ntag = rfid->uid.sak == 0x00 && isNtagSize(rfid->totalPages);
    out.type = out.ntag ? ntagName(rfid->totalPages) : rfid->nfc.PICC_GetTypeName(rfid->uid.sak);
    if (!out.ntag || !rfid->pageReadSuccess) return;
    std::vector<uint8_t> mem = pagesFromDump(rfid->strAllPages, rfid->totalPages);
    out.capacity = capacityFor(rfid->totalPages);
    out.locked = mem[10] || mem[11]; // static lock bytes (page 2, bytes 2-3)
    out.uri = parseNdefUri(mem);
}

} // namespace

const char *cardStatusText(CardStatus s) {
    switch (s) {
        case CARD_OK: return "OK";
        case CARD_NONE: return "No card";
        case CARD_NO_READER: return "Turn DIP 3 ON";
        case CARD_BUSY: return "Reader busy";
        case CARD_NOT_NTAG: return "Not an NTAG213/215/216 tag";
        case CARD_TOO_LONG: return "URL too long for this tag";
        case CARD_LOCKED: return "Tag is write-locked";
        case CARD_WRITE_FAILED: return "Write failed - hold still";
        case CARD_VERIFY_FAILED: return "Verify failed";
    }
    return "?";
}

CardStatus readerOpen() {
    if (xSemaphoreTake(readerMutex(), 0) != pdTRUE) return CARD_BUSY;
    held = true;
    HatProbeResult hat = probeHat();
    if (!hat.pn532) {
        readerClose();
        return CARD_NO_READER;
    }
    if (!rfid) rfid = new PN532(PN532::CONNECTION_TYPE::I2C);
    if (!rfid->begin()) {
        readerClose();
        return CARD_NO_READER;
    }
    rfid->nfc.SAMConfig();
    // Answer "no tag" after a few tries instead of waiting forever, so a poll
    // never leaves a command pending in the chip.
    rfid->nfc.setPassiveActivationRetries(0x05);
    return CARD_OK;
}

void readerClose() {
    if (!held) return;
    held = false;
    xSemaphoreGive(readerMutex());
}

CardStatus pollCard(CardInfo &out) {
    if (!held || !rfid) return CARD_NO_READER;
    // 150 ms: long enough for the PN532's "no card" answer after its 5 tries, so
    // the next poll never talks over a command still running.
    if (!selectCard(150)) return CARD_NONE;
    if (readFull() != RFIDInterface::SUCCESS) return CARD_NONE;
    fill(out);
    return CARD_OK;
}

CardStatus waitCard(
    CardInfo &out, uint32_t timeoutMs, const std::function<bool()> &cancel, const std::function<void()> &idle
) {
    uint32_t start = millis();
    for (;;) {
        CardStatus s = pollCard(out);
        if (s != CARD_NONE) return s;
        if (cancel && cancel()) return CARD_NONE;
        if (timeoutMs && millis() - start > timeoutMs) return CARD_NONE;
        if (idle) idle();
        delay(10);
    }
}

CardStatus writeCard(const String &uri, CardInfo &out) {
    if (!held || !rfid) return CARD_NO_READER;
    CardInfo before;
    if (!selectCard(200) || readFull() != RFIDInterface::SUCCESS) return CARD_NONE;
    fill(before);
    if (!before.ntag) return CARD_NOT_NTAG;
    if (before.locked) return CARD_LOCKED;

    // rawNdefRecord, not buildNdefMessage(): that caps URIs at 99 characters
    // and a signed check-in URL is ~100.
    rfid->rawNdefRecord = uriRecord(uri);
    if (rfid->rawNdefRecord.size() > 254 || (int)rfid->rawNdefRecord.size() + 3 > before.capacity) {
        rfid->rawNdefRecord.clear();
        return CARD_TOO_LONG;
    }
    int r = RFIDInterface::FAILURE;
    for (int i = 0; i < 4 && r != RFIDInterface::SUCCESS; i++) {
        // write_ndef() would re-select without waiting (see readSelected()).
        if (!selectCard(200)) {
            delay(20);
            continue;
        }
        rfid->set_uid();
        r = rfid->write_ndef_blocks();
        if (r == RFIDInterface::TAG_NOT_MATCH) break;
        if (r != RFIDInterface::SUCCESS) delay(20);
    }
    rfid->rawNdefRecord.clear(); // Bruce's own RFID screens share nothing with us, but be tidy
    if (r == RFIDInterface::TAG_NOT_MATCH) return CARD_NOT_NTAG;
    if (r != RFIDInterface::SUCCESS) return CARD_WRITE_FAILED;

    if (!selectCard(200) || readFull() != RFIDInterface::SUCCESS) return CARD_VERIFY_FAILED;
    fill(out);
    if (out.uid != before.uid || out.uri != uri) return CARD_VERIFY_FAILED;
    flashSuccess(); // no speaker on this board: the light says it worked
    return CARD_OK;
}

CardStatus lockCard(const String &expectUid) {
    if (!held || !rfid) return CARD_NO_READER;
    CardInfo now;
    if (!selectCard(200) || readFull() != RFIDInterface::SUCCESS) return CARD_NONE;
    fill(now);
    if (!now.ntag) return CARD_NOT_NTAG;
    if (now.uid != expectUid) return CARD_VERIFY_FAILED; // a different tag got swapped in
    if (now.locked) return CARD_OK;
    std::vector<uint8_t> mem = pagesFromDump(rfid->strAllPages, rfid->totalPages);
    // NTAG21x datasheet: dynamic lock bytes are the page before CFG0 (bytes
    // 0-2; byte 3 is RFUI and kept), static lock bytes are page 2 bytes 2-3.
    // Dynamic first: the static bits also freeze the block-lock bits.
    int dyn = rfid->totalPages - 5;
    uint8_t d[4] = {0xFF, 0xFF, 0xFF, mem[dyn * 4 + 3]};
    uint8_t st[4] = {mem[8], mem[9], 0xFF, 0xFF};
    if (!rfid->nfc.ntag2xx_WritePage(dyn, d)) return CARD_WRITE_FAILED;
    if (!rfid->nfc.ntag2xx_WritePage(2, st)) return CARD_WRITE_FAILED;
    if (!selectCard(200) || readFull() != RFIDInterface::SUCCESS) return CARD_VERIFY_FAILED;
    fill(now);
    return now.locked ? CARD_OK : CARD_VERIFY_FAILED;
}

} // namespace freq
