#ifndef PN532_NFC_MANAGER_H
#define PN532_NFC_MANAGER_H

#if SCREEN_SIZE == 7

#include <Arduino.h>
#include <Preferences.h>

// NFC tag record — maps a physical tag UID to a Spotify URI
struct NFCTag {
    String uid;
    String spotifyUri;
    String spotifyName;
    String artist;
    uint8_t scanCount;
    uint32_t lastScan;
};

class PN532_NFCManager {
public:
    PN532_NFCManager() {}
    ~PN532_NFCManager() {}

    // Initialise hardware — I2C SDA/SCL (expansion connector pins 21/23 = GPIO7/GPIO8)
    bool begin(uint8_t sdaPin = 7, uint8_t sclPin = 8);

    // Call from main loop — polls for new tags
    void update();

    // Called when a tag is scanned (triggers Spotify playback)
    void onTagScanned(const String& uid, const String& uri, const String& name);

    // Tag detection state
    bool isTagPresent()            { return _tagPresent; }
    String getCurrentTagUID()      { return _currentUID; }
    String getCurrentTagName()     { return _currentName; }
    String getCurrentTagUri()      { return _currentUri; }

    // Connection status
    bool isConnected()             { return _connected; }
    String getFirmwareVersionStr() { return _fwVersion; }

    // Tag registry
    bool addTag(const String& uid, const String& name, const String& uri);
    void removeTag(const String& uid);
    void listTags();

    // Scanning control — also used by NFC screen toggle to re-init hardware
    void setScanning(bool enabled) { _scanning = enabled; }
    bool isScanning()              { return _scanning; }
    // Re-initialise hardware (called when the enabled toggle is flipped on)
    bool reconnect(uint8_t sdaPin = 7, uint8_t sclPin = 8) { return begin(sdaPin, sclPin); }

private:
    Preferences _prefs;

    bool   _connected   = false;
    String _fwVersion   = "";

    bool   _tagPresent = false;
    String _currentUID  = "";
    String _currentName = "";
    String _currentUri  = "";
    bool   _scanning    = true;

    uint8_t _rxPin = 7;   // SDA — expansion connector pin 21 (GPIO7)
    uint8_t _txPin = 8;   // SCL — expansion connector pin 23 (GPIO8)

    static const uint8_t MAX_TAGS = 50;
    NFCTag   _tags[MAX_TAGS];
    uint8_t  _tagCount = 0;

    uint32_t _lastScanTime  = 0;
    const uint32_t SCAN_INTERVAL = 500;

    String uidToString(uint8_t* uid, uint8_t length);
    int    findTagIndex(const String& uid);
    bool   loadTags();
    bool   saveTags();
    // Read NDEF Text record from a Mifare Ultralight / NTAG tag (pages 4+, 7-byte UID)
    String readNDEFText();
    // Read NDEF Text record from a Mifare Classic tag (4-byte UID, SAK=0x08)
    String readNDEFTextMifareClassic(uint8_t* uid, uint8_t uidLen);
};

// Alias so any new code using the shorter name also works
typedef PN532_NFCManager NFCManager;

#endif // SCREEN_SIZE == 7
#endif // PN532_NFC_MANAGER_H
