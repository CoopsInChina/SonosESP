#include "pn532_nfc_manager.h"

#if SCREEN_SIZE == 7

#include "PN532_I2C.h"
#include "PN532.h"
#include <Wire.h>
#include "node_sonos_server.h"
#include "sonos_controller.h"

extern SonosController sonos;

// ---------------------------------------------------------------------------
// PN532_NFCManager — I2C implementation
//
// Hardware wiring (I2C mode on Waveshare PN532 HAT):
//   DIP switch 1 = ON  (I0=H)
//   DIP switch 2 = OFF (I1=L)
//
//   Board SDA  →  Expansion pin 21 (GPIO7  = ES_I2C_SDA, shared with ES8311 codec)
//   Board SCL  →  Expansion pin 23 (GPIO8  = ES_I2C_SCL, shared with ES8311 codec)
//   Board GND  →  Expansion pin 16 (GND)
//   Board 3V3  →  Expansion pin 18 (VCC3V3)
//
//   The Waveshare board has built-in pull-up resistors — no external resistors needed.
//   PN532 I2C address: 0x24 (does not conflict with ES8311 at 0x18)
// ---------------------------------------------------------------------------

static PN532_I2C* _i2c    = nullptr;
static PN532*     _nfc532 = nullptr;

bool PN532_NFCManager::begin(uint8_t sdaPin, uint8_t sclPin) {
    _rxPin = sdaPin;  // reused as SDA
    _txPin = sclPin;  // reused as SCL

    Serial.printf("[NFC] begin() — I2C SDA=GPIO%d SCL=GPIO%d addr=0x24\n", sdaPin, sclPin);

    // Initialise I2C bus (shared with ES8311 audio codec — same pins, different address)
    Wire.begin(sdaPin, sclPin);
    Wire.setClock(100000);  // 100kHz — conservative for PN532

    // Quick bus scan to confirm something is at 0x24
    Serial.println("[NFC] Scanning I2C bus...");
    bool found_nfc = false;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        uint8_t err = Wire.endTransmission();
        if (err == 0) {
            Serial.printf("[NFC]   Device found at 0x%02X%s\n", addr,
                          addr == 0x24 ? " ← PN532" :
                          addr == 0x18 ? " ← ES8311 codec" : "");
            if (addr == 0x24) found_nfc = true;
        }
    }
    if (!found_nfc) {
        Serial.println("[NFC] ERROR: Nothing at 0x24 — check wiring and DIP switches");
        Serial.println("[NFC]   I2C mode: DIP switch 1=ON (I0=H), switch 2=OFF (I1=L)");
        Serial.println("[NFC]   Then fully power-cycle the board");
        _connected = false;
        _fwVersion = "";
        return false;
    }

    _i2c    = new PN532_I2C(Wire);
    _nfc532 = new PN532(*_i2c);

    // PN532 needs settling time after power-on
    delay(100);

    uint32_t versiondata = 0;
    for (int attempt = 1; attempt <= 3 && !versiondata; attempt++) {
        Serial.printf("[NFC] getFirmwareVersion attempt %d/3...\n", attempt);
        _i2c->wakeup();
        _nfc532->begin();
        versiondata = _nfc532->getFirmwareVersion();
        if (!versiondata) delay(200);
    }

    if (!versiondata) {
        Serial.println("[NFC] ERROR: PN532 at 0x24 found but getFirmwareVersion failed");
        Serial.println("[NFC]   Confirm DIP switches set correctly and power-cycled");
        delete _nfc532; _nfc532 = nullptr;
        delete _i2c;    _i2c    = nullptr;
        _connected = false;
        _fwVersion = "";
        return false;
    }

    char fwBuf[24];
    snprintf(fwBuf, sizeof(fwBuf), "PN5%02X FW %d.%d",
        (versiondata >> 24) & 0xFF,
        (versiondata >> 16) & 0xFF,
        (versiondata >>  8) & 0xFF);
    _fwVersion = String(fwBuf);
    _connected = true;

    Serial.printf("[NFC] %s\n", fwBuf);

    _nfc532->SAMConfig();
    Serial.println("[NFC] Ready — scanning for ISO14443A tags");

    loadTags();
    return true;
}

void PN532_NFCManager::update() {
    if (!_scanning || !_nfc532) return;
    if (millis() - _lastScanTime < SCAN_INTERVAL) return;
    _lastScanTime = millis();

    uint8_t uid[7];
    uint8_t uidLen = 0;

    bool found = _nfc532->readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen, 100);

    if (found && uidLen > 0) {
        String uidStr = uidToString(uid, uidLen);

        if (!_tagPresent || uidStr != _currentUID) {
            // Route to the right NDEF reader based on tag type:
            //   4-byte UID = Mifare Classic (SAK 0x08) — requires block auth
            //   7-byte UID = NTAG / Mifare Ultralight — page reads
            String ndefText = (uidLen == 4)
                ? readNDEFTextMifareClassic(uid, uidLen)
                : readNDEFText();

            // NTAG reads can fail if the tag isn't fully settled — retry up to 3 times
            for (int retry = 0; retry < 3 && ndefText.length() == 0; retry++) {
                vTaskDelay(pdMS_TO_TICKS(60));
                ndefText = (uidLen == 4)
                    ? readNDEFTextMifareClassic(uid, uidLen)
                    : readNDEFText();
                if (ndefText.length() > 0)
                    Serial.printf("[NFC] NDEF read succeeded on retry %d\n", retry + 1);
            }

            if (ndefText.length() > 0) {
                // Parse "Service:Type:ID" — e.g. "spotify:album:2dfTV7CktUEBkZCHiB7VQB"
                // Reconstruct full URI as "service:type:id"
                Serial.printf("[NFC] NDEF text: %s\n", ndefText.c_str());
                // First colon splits service from the rest ("type:id")
                int c1 = ndefText.indexOf(':');
                int c2 = (c1 >= 0) ? ndefText.indexOf(':', c1 + 1) : -1;
                String service = (c1 >= 0) ? ndefText.substring(0, c1) : ndefText;
                String typeId  = (c2 >= 0) ? ndefText.substring(c1 + 1) : "";  // "album:id"
                String name    = (c2 >= 0) ? ndefText.substring(c1 + 1, c2) + " " + service : service;
                // URI is the full colon-separated string (spotify:album:xxx)
                onTagScanned(uidStr, ndefText, name);
            } else {
                // No NDEF — check registry
                int idx = findTagIndex(uidStr);
                if (idx >= 0) {
                    Serial.printf("[NFC] Known tag: uid=%s  name=%s  uri=%s\n",
                        uidStr.c_str(),
                        _tags[idx].spotifyName.c_str(),
                        _tags[idx].spotifyUri.c_str());
                    onTagScanned(uidStr, _tags[idx].spotifyUri, _tags[idx].spotifyName);
                } else {
                    Serial.printf("[NFC] Unknown tag: uid=%s (no NDEF, not in registry)\n", uidStr.c_str());
                    _tagPresent = true;
                    _currentUID  = uidStr;
                    _currentUri  = "";
                    _currentName = "";
                }
            }
        }
    } else {
        if (_tagPresent) {
            Serial.println("[NFC] Tag removed");
        }
        _tagPresent = false;
    }
}

void PN532_NFCManager::onTagScanned(const String& uid, const String& uri, const String& name) {
    _tagPresent  = true;
    _currentUID  = uid;
    _currentUri  = uri;
    _currentName = name;
    Serial.printf("[NFC] Tag scanned: uid=%s  name=%s  uri=%s\n",
                  uid.c_str(), name.c_str(), uri.c_str());

    // Get the currently selected Sonos room and fire the play request
    String room;
    SonosDevice* dev = sonos.getCurrentDevice();
    if (dev) room = dev->roomName;
    sonosHttpServer.sendPlayRequest(uri, room);
}

bool PN532_NFCManager::addTag(const String& uid, const String& name, const String& uri) {
    if (_tagCount >= MAX_TAGS) {
        Serial.println("[NFC] Tag registry full");
        return false;
    }
    int idx = findTagIndex(uid);
    if (idx >= 0) {
        _tags[idx].spotifyUri  = uri;
        _tags[idx].spotifyName = name;
        saveTags();
        return true;
    }
    NFCTag t;
    t.uid         = uid;
    t.spotifyUri  = uri;
    t.spotifyName = name;
    t.scanCount   = 0;
    t.lastScan    = 0;
    _tags[_tagCount++] = t;
    saveTags();
    Serial.printf("[NFC] Tag added: uid=%s  name=%s\n", uid.c_str(), name.c_str());
    return true;
}

void PN532_NFCManager::removeTag(const String& uid) {
    int idx = findTagIndex(uid);
    if (idx < 0) return;
    for (int i = idx; i < _tagCount - 1; i++) _tags[i] = _tags[i + 1];
    _tagCount--;
    saveTags();
}

void PN532_NFCManager::listTags() {
    Serial.printf("[NFC] Registered tags (%d):\n", _tagCount);
    for (int i = 0; i < _tagCount; i++) {
        Serial.printf("  [%d] uid=%s  name=%s  uri=%s\n",
                      i, _tags[i].uid.c_str(),
                      _tags[i].spotifyName.c_str(),
                      _tags[i].spotifyUri.c_str());
    }
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

String PN532_NFCManager::uidToString(uint8_t* uid, uint8_t length) {
    String s;
    for (int i = 0; i < length; i++) {
        if (uid[i] < 0x10) s += "0";
        s += String(uid[i], HEX);
    }
    s.toUpperCase();
    return s;
}

int PN532_NFCManager::findTagIndex(const String& uid) {
    for (int i = 0; i < _tagCount; i++) {
        if (_tags[i].uid == uid) return i;
    }
    return -1;
}

bool PN532_NFCManager::loadTags() {
    // TODO: restore tag registry from NVS via _prefs
    return true;
}

bool PN532_NFCManager::saveTags() {
    // TODO: persist tag registry to NVS via _prefs
    return true;
}

// Read an NDEF Text record from a Mifare Classic 1K tag (4-byte UID, SAK=0x08).
// NDEF data lives in sectors 1+ authenticated with the NFC Forum NDEF key.
// Each sector has 4 blocks of 16 bytes; block 3 of each sector is the trailer (skip it).
String PN532_NFCManager::readNDEFTextMifareClassic(uint8_t* uid, uint8_t uidLen) {
    if (!_nfc532) return "";

    // Try keys in order: NFC Forum NDEF key, then factory default, then MAD key
    uint8_t candidateKeys[][6] = {
        {0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7},  // NFC Forum NDEF key
        {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},    // Factory default
        {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5},    // MAD key (sector 0)
    };
    const int numKeys = 3;

    // Probe sector 1 to find which key works
    uint8_t* workingKey = nullptr;
    for (int k = 0; k < numKeys; k++) {
        if (_nfc532->mifareclassic_AuthenticateBlock(uid, uidLen, 4, 0, candidateKeys[k])) {
            workingKey = candidateKeys[k];
            Serial.printf("[NFC] Mifare Classic: sector 1 auth OK with key[%d]\n", k);
            break;
        }
    }
    if (!workingKey) {
        Serial.println("[NFC] Mifare Classic: sector 1 auth failed with all keys");
        return "";
    }

    // Collect up to 3 sectors of user data (sectors 1–3 = blocks 4–15)
    // Each sector yields 3 data blocks × 16 bytes = 48 bytes
    uint8_t allData[144];
    int dataLen = 0;

    for (int sector = 1; sector <= 3 && dataLen < (int)sizeof(allData); sector++) {
        int firstBlock = sector * 4;

        // Authenticate using the key that worked for sector 1
        if (!_nfc532->mifareclassic_AuthenticateBlock(uid, uidLen, firstBlock, 0, workingKey)) {
            break;  // Stop if a later sector can't be authenticated
        }

        // Read the 3 data blocks (block offset 3 is the sector trailer — skip it)
        for (int blk = 0; blk < 3; blk++) {
            uint8_t buf[16];
            if (!_nfc532->mifareclassic_ReadDataBlock(firstBlock + blk, buf)) {
                Serial.printf("[NFC] Mifare Classic: read failed at block %d\n", firstBlock + blk);
                break;
            }
            memcpy(allData + dataLen, buf, 16);
            dataLen += 16;
        }

    }

    if (dataLen < 2) return "";

    // The TLV stream on Mifare Classic starts at byte 0 of the first data block.
    // Parse identically to the NTAG path.
    int i = 0;
    while (i < dataLen) {
        uint8_t tlvType = allData[i++];
        if (tlvType == 0xFE) break;
        if (tlvType == 0x00) continue;

        if (i >= dataLen) break;
        uint16_t tlvLen;
        if (allData[i] == 0xFF) {
            i++;
            if (i + 2 > dataLen) break;
            tlvLen = ((uint16_t)allData[i] << 8) | allData[i + 1];
            i += 2;
        } else {
            tlvLen = allData[i++];
        }

        if (tlvType != 0x03) { i += tlvLen; continue; }

        int ndefEnd = i + tlvLen;
        while (i < ndefEnd && i < dataLen) {
            uint8_t hdr       = allData[i++];
            bool    sr        = (hdr & 0x10) != 0;
            bool    il        = (hdr & 0x08) != 0;
            uint8_t tnf       = hdr & 0x07;
            uint8_t typeLen   = allData[i++];

            uint32_t payloadLen;
            if (sr) {
                payloadLen = allData[i++];
            } else {
                payloadLen = ((uint32_t)allData[i] << 24) | ((uint32_t)allData[i+1] << 16) |
                             ((uint32_t)allData[i+2] << 8) | allData[i+3];
                i += 4;
            }
            if (il) { uint8_t idLen = allData[i++]; i += idLen; }

            char recType[8] = {};
            for (int t = 0; t < typeLen && t < 7 && i < dataLen; t++) recType[t] = (char)allData[i++];

            if (tnf == 0x01 && typeLen == 1 && recType[0] == 'T' && payloadLen > 0) {
                uint8_t statusByte = allData[i++];
                uint8_t langLen    = statusByte & 0x3F;
                i += langLen;
                uint32_t textLen   = payloadLen - 1 - langLen;
                String text = "";
                for (uint32_t t = 0; t < textLen && (i + (int)t) < dataLen; t++)
                    text += (char)allData[i + t];
                return text;
            }
            i += (int)payloadLen;
        }
        break;
    }
    return "";
}

// Read an NDEF Text record from a Mifare Ultralight / NTAG tag.
// User memory starts at page 4 (4 bytes per page).
// Returns the text payload, or "" if not found.
String PN532_NFCManager::readNDEFText() {
    if (!_nfc532) return "";

    // Read 16 pages (64 bytes) starting at page 4 (covers NTAG213 / 215 / 216 user memory start)
    uint8_t data[64];
    int dataLen = 0;
    for (uint8_t page = 4; page < 20 && dataLen < 64; page++) {
        uint8_t buf[4];
        if (_nfc532->mifareultralight_ReadPage(page, buf) != 1) break;
        memcpy(data + dataLen, buf, 4);
        dataLen += 4;
    }
    if (dataLen < 2) return "";

    // Parse TLV stream (Type-Length-Value containers wrapping the NDEF message)
    int i = 0;
    while (i < dataLen) {
        uint8_t tlvType = data[i++];
        if (tlvType == 0xFE) break;   // Terminator TLV — end of data
        if (tlvType == 0x00) continue; // Null TLV — padding, skip

        // Decode length (3-byte form if first byte is 0xFF)
        if (i >= dataLen) break;
        uint16_t tlvLen;
        if (data[i] == 0xFF) {
            i++;
            if (i + 2 > dataLen) break;
            tlvLen = ((uint16_t)data[i] << 8) | data[i + 1];
            i += 2;
        } else {
            tlvLen = data[i++];
        }

        if (tlvType != 0x03) {  // 0x03 = NDEF Message TLV
            i += tlvLen;
            continue;
        }

        // --- NDEF Message TLV found ---
        if (i + (int)tlvLen > dataLen) break;
        int ndefEnd = i + tlvLen;

        while (i < ndefEnd) {
            if (i >= dataLen) break;
            uint8_t hdr = data[i++];
            bool    sr  = (hdr & 0x10) != 0;  // Short Record (payload len = 1 byte)
            bool    il  = (hdr & 0x08) != 0;  // ID Length field present
            uint8_t tnf = hdr & 0x07;

            if (i >= dataLen) break;
            uint8_t typeLen = data[i++];

            uint32_t payloadLen;
            if (sr) {
                if (i >= dataLen) break;
                payloadLen = data[i++];
            } else {
                if (i + 4 > dataLen) break;
                payloadLen = ((uint32_t)data[i] << 24) | ((uint32_t)data[i+1] << 16) |
                             ((uint32_t)data[i+2] << 8) | data[i+3];
                i += 4;
            }

            if (il) {  // Skip ID Length + ID bytes
                if (i >= dataLen) break;
                uint8_t idLen = data[i++];
                i += idLen;
            }

            // Read type bytes
            char recType[8] = {};
            for (int t = 0; t < typeLen && t < 7 && i < dataLen; t++) {
                recType[t] = (char)data[i++];
            }

            // We only care about Well Known Text records (TNF=0x01, type="T")
            if (tnf == 0x01 && typeLen == 1 && recType[0] == 'T' && payloadLen > 0) {
                if (i >= dataLen) break;
                uint8_t statusByte = data[i++];
                uint8_t langLen = statusByte & 0x3F;
                i += langLen;  // skip language code (e.g. "en")
                uint32_t textLen = payloadLen - 1 - langLen;

                String text = "";
                for (uint32_t t = 0; t < textLen && (i + (int)t) < dataLen; t++) {
                    text += (char)data[i + t];
                }
                return text;
            }

            // Not a text record — skip payload and move to next record
            i += (int)payloadLen;
        }
        break;  // Only one NDEF TLV expected
    }
    return "";
}

#endif // SCREEN_SIZE == 7
